#include <accvm/context.h>
#include <accvm/thread.h>
#include <accvm/accvm.h>
#include <accvm/acctime.h>
#include <config/config.h>

#include <stdint.h>
#include <errno.h>
#include <bpf/bpf.h>

#define BPF_MAP_UPDATE 0 
#define BPF_MAP_LOOKUP 1
#define BPF_MAP_LOOKUP_DELETE 2

#define JAIL_BREAK_NO_HALT 0x7000
#define JAIL_NO_HALT 0x8000

struct pstate {
    uint64_t sim_state;
    uint64_t epoch_dur;
    uint32_t pin_cpu;
    uint32_t ctrl_msg;
    bool jailbreak;
    uint64_t reversed_priority;
};

typedef struct __attribute__((packed)) SchedRegs {
  uint64_t ctrl;
  uint64_t lock;
} SchedRegs;

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

int ebs_is_on(){
  __u32 index = 0;
  __u64 state = 0;
  bpf_map_lookup_elem(bpf_sched_ctrl_fd, &index, &state);
  return state == 1;
}

void tick_nex() {
  __asm__ volatile("ud2");
}

void bpf_sched_update_state_per_pid(uint32_t ctrl_pid, uint32_t ctrl_msg){
  struct pstate state;
  put_bpf_map(sim_proc_state_fd, &ctrl_pid, &state, BPF_MAP_LOOKUP);
  state.ctrl_msg = ctrl_msg;
  put_bpf_map(sim_proc_state_fd, &ctrl_pid, &state, BPF_MAP_UPDATE);
}

static int sched_my_thread_id(void) {
  return syscall(SYS_gettid);
}

static void sched_ctrl_lock(SchedRegs* sched_reg) {
    // Acquire lock using atomic compare-and-swap
    while (__sync_val_compare_and_swap(&sched_reg->lock, 0, 1) != 0) {
        // Busy wait / spin
        sched_yield();
    }
}

static void sched_ctrl_unlock(SchedRegs* sched_reg) {
    // Release lock
    __sync_lock_release(&sched_reg->lock);
}

void update_ctrl_reg(uint64_t value) {
    SchedRegs* sched_reg = (SchedRegs *)ctrl_base;
    
    sched_ctrl_lock(sched_reg);
    // even safer, atomic store
    sched_reg->ctrl = (uint64_t)sched_my_thread_id() << 32 | value;
    tick_nex();
    sched_ctrl_unlock(sched_reg);
}

int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine)(void *),
    void *arg
) {
  
  // #if CONFIG_USE_FAULT
    // USE FAULT requires reading process memory to decode instructions
    // the scheduler control logic conflicts with thread creation when segfaults happens
    // which deadlocks the whole system
    // the beflow is to address that issue
    if(ebs_is_on()){
        // printf("pthread_create: EBS is on, updating state for jail break\n");
        update_ctrl_reg(JAIL_BREAK_NO_HALT);
        int ret = orig_pthread_create(thread, attr, start_routine, arg);
        update_ctrl_reg(JAIL_NO_HALT);
        // printf("pthread_create: Exit jail break\n");
        return ret;
        
    }else{
        return orig_pthread_create(thread, attr, start_routine, arg);
    }
  // #else
  //   return orig_pthread_create(thread, attr, start_routine, arg);
  // #endif


    /* ─── custom logic after calling real pthread_create() ────────────────── */
    /* e.g. re-enable SCX control, record the pthread_t in a map, etc. */

}
