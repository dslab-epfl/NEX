#include <config/config.h>

#if CONFIG_ENABLE_BPF
#include <bits/stdint-uintn.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <config/config.h>
#include <exec/bpf.h>
#include <scx/common.h>
#include "scx_simple.bpf.skel.h"

static bool verbose;
static volatile int exit_req;

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	if (level == LIBBPF_DEBUG && !verbose)
		return 0;
	return vfprintf(stderr, format, args);
}

static void sigint_handler(int simple)
{
	exit_req = 1;
}

static struct scx_simple *skel;
static struct bpf_link *elink;
static int map_fd;

int rewrite_bpf(int pid){
	__u32 index = 0;
	if (bpf_map_update_elem(map_fd, &index, &pid, 0) != 0) {
        fprintf(stderr, "ERROR: updating BPF map: %s\n", strerror(errno));
        scx_simple__destroy(skel);
        return 1;
    }
	printf("PID %u written to BPF map\n", pid);
	return 0;
}


uint64_t read_vts(){
	__u32 index = 0;
	__u64 vts = 0;
	int fd = bpf_map__fd(skel->maps.vts);
	bpf_map_lookup_elem(fd, &index, &vts);
	return vts;
}

struct sched_stats{
	__u64 err_early_end;
	__u64 err_late_end;
};

uint64_t read_err_bound(){
	__u32 index = 0;
	struct sched_stats stats;
	int fd = bpf_map__fd(skel->maps.sched_stats);
	bpf_map_lookup_elem(fd, &index, &stats);
	return stats.err_late_end;
}

int pop_traced_child(int *ret_pid){
	__u32 pid = 0;
	int fd = bpf_map__fd(skel->maps.traced_q);
	int ret = bpf_map_lookup_and_delete_elem(fd, NULL, &pid);
	*ret_pid = (int)pid; 
	return ret;
}

#define UNPIN_BPF_MAP_PATH(map_name, path_name) \
    int map_name##_fd = bpf_map__fd(skel->maps.map_name); \
    if (map_name##_fd < 0) { \
        fprintf(stderr, "Error finding BPF map\n"); \
        return 1; \
    } \
    if (access(path_name, F_OK) != -1) { \
        if (unlink(path_name) < 0) { \
            fprintf(stderr, "Error unpinning existing BPF map: %s\n", strerror(errno)); \
            return 1; \
        } \
    } else { \
        fprintf(stderr, "Map not found at %s, nothing to unpin.\n", path_name); \
    }

#define PIN_BPF_MAP_PATH(map_name, path_name) \
    int	map_name##_fd = bpf_map__fd(skel->maps.map_name); \
    if (map_name##_fd < 0) { \
        fprintf(stderr, "Error finding BPF map\n"); \
        return 1; \
    } \
	if (access(path_name, F_OK) != -1) { \
        if (unlink(path_name) < 0) { \
            fprintf(stderr, "Error unpinning existing BPF map: %s\n", strerror(errno)); \
            return 1; \
        }\
    }\
    if (bpf_obj_pin(map_name##_fd, path_name)) { \
        fprintf(stderr, "Error pinning BPF map %d\n", errno); \
        return 1; \
    } \

int attach_bpf(int pid){
	
	libbpf_set_print(libbpf_print_fn);
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);

	skel = SCX_OPS_OPEN(simple_ops, scx_simple);
	
	skel->rodata->NORMAL_QUANTUM = 20*1000000;
	skel->rodata->SWITCH_WAIT_TIME = 0;
	skel->rodata->target_pid = pid;

	#if CONFIG_ROUND_BASED_MODE
	// 2us 850ns
	// 1us 750ns
	skel->rodata->EXTRA_COST_TIME = CONFIG_EXTRA_COST_TIME;
	skel->rodata->TIME_QUANTUM = CONFIG_ROUND_SLICE;
	skel->rodata->NR_CORES = CONFIG_TOTAL_CORES;
	skel->rodata->SIM_NR_CORES = CONFIG_SIM_CORES;
	skel->rodata->DEFAULT_ON_OFF = CONFIG_DEFAULT_ON_OFF;
#if CONFIG_SIM_PHY_CORES > 0
	skel->rodata->SIM_PHY_CORES = CONFIG_SIM_PHY_CORES;
#endif
	#if CONFIG_EAGER_SYNC
	skel->rodata->EAGER_SYNC = CONFIG_EAGER_SYNC;
	skel->rodata->EAGER_SYNC_PERIOD = CONFIG_EAGER_SYNC_PERIOD;
#endif
	#endif

	SCX_OPS_LOAD(skel, simple_ops, scx_simple, uei);

#if CONFIG_EAGER_SYNC
	PIN_BPF_MAP_PATH(from_nex_runtime_event_q, "/sys/fs/bpf/from_nex_runtime_event_q");
	PIN_BPF_MAP_PATH(to_nex_runtime_event_q, "/sys/fs/bpf/to_nex_runtime_event_q");
#endif

	PIN_BPF_MAP_PATH(vts, "/sys/fs/bpf/vts");
	PIN_BPF_MAP_PATH(event_q, "/sys/fs/bpf/event_q");
	// PIN_BPF_MAP_PATH(expired_event_q, "/sys/fs/bpf/expired_event_q");
	PIN_BPF_MAP_PATH(thread_state_map, "/sys/fs/bpf/thread_state_map");
	PIN_BPF_MAP_PATH(syscall_entry_real_time_map, "/sys/fs/bpf/syscall_entry_real_time_map");
	PIN_BPF_MAP_PATH(bpf_sched_ctrl, "/sys/fs/bpf/bpf_sched_ctrl");
	PIN_BPF_MAP_PATH(trace_event_q, "/sys/fs/bpf/trace_event_q");

	PIN_BPF_MAP_PATH(sim_proc_state, "/sys/fs/bpf/sim_proc_state")

	printf("PID %u written to BPF map\n", pid);
      
	elink = SCX_OPS_ATTACH(skel, simple_ops, scx_simple);

	return 0;
}

int destroy_bpf()
{
#if CONFIG_EAGER_SYNC
	UNPIN_BPF_MAP_PATH(from_nex_runtime_event_q, "/sys/fs/bpf/from_nex_runtime_event_q");
	UNPIN_BPF_MAP_PATH(to_nex_runtime_event_q, "/sys/fs/bpf/to_nex_runtime_event_q");
#endif	

	UNPIN_BPF_MAP_PATH(vts, "/sys/fs/bpf/vts");
	UNPIN_BPF_MAP_PATH(event_q, "/sys/fs/bpf/event_q");
	// UNPIN_BPF_MAP_PATH(expired_event_q, "/sys/fs/bpf/expired_event_q");
	UNPIN_BPF_MAP_PATH(thread_state_map, "/sys/fs/bpf/thread_state_map");
	UNPIN_BPF_MAP_PATH(syscall_entry_real_time_map, "/sys/fs/bpf/syscall_entry_real_time_map");
	UNPIN_BPF_MAP_PATH(bpf_sched_ctrl, "/sys/fs/bpf/bpf_sched_ctrl");
	UNPIN_BPF_MAP_PATH(trace_event_q, "/sys/fs/bpf/trace_event_q");
	UNPIN_BPF_MAP_PATH(sim_proc_state, "/sys/fs/bpf/sim_proc_state");
	
	bpf_link__destroy(elink);
	UEI_REPORT(skel, uei);
	scx_simple__destroy(skel);

	return 0;
}

int get_bpf_map(char* map_name){
	char path_name[100] = "/sys/fs/bpf/";
	strcat(path_name, map_name);
	int fd = bpf_obj_get(path_name);
	if (fd < 0) {
		fprintf(stderr, "Error finding BPF map %s\n", path_name);
		return 1;
	}
	return fd;
}

int put_bpf_map(int map_fd, void* key, void* value, int ops){
	if(ops == BPF_MAP_UPDATE){
		if (bpf_map_update_elem(map_fd, key, value, BPF_ANY) != 0) {
			fprintf(stderr, "ERROR: updating BPF map: %s\n", strerror(errno));
			return 1;
		}
	}
	else if(ops == BPF_MAP_LOOKUP){
		if (bpf_map_lookup_elem(map_fd, key, value) != 0) {
			fprintf(stderr, "ERROR: lookup BPF map: %s\n", strerror(errno));
			return 1;
		}
	}
	else if(ops == BPF_MAP_LOOKUP_DELETE){
		if (bpf_map_lookup_and_delete_elem(map_fd, key, value) != 0) {
			// fprintf(stderr, "ERROR: lookup delete BPF map: %s\n", strerror(errno));
			return 1;
		}
	}
	return 0;
}

#endif
