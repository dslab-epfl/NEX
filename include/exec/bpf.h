#pragma once
#include <config/config.h>
#include <stdint.h>
#include <stdbool.h>
#if CONFIG_ENABLE_BPF
#include <bpf/bpf.h>
#endif

#define BPF_MAP_UPDATE 0 
#define BPF_MAP_LOOKUP 1
#define BPF_MAP_LOOKUP_DELETE 2


struct per_thread_state {
    uint64_t halt_until;
};

struct pstate {
    uint64_t sim_state;
    uint64_t epoch_dur;
    uint32_t pin_cpu;
    uint32_t ctrl_msg;
    bool jailbreak;
    uint64_t reversed_priority;
};

struct custom_event{
    uint32_t type;
    uint64_t ts;
    uint64_t data;
};

extern int trace_evnt_fd; // removed 

extern int from_nex_runtime_event_q_fd;
extern int to_nex_runtime_event_q_fd;
extern int sim_proc_state_fd;

extern uint64_t read_vts();
extern int get_bpf_map(char* map_name);
extern int put_bpf_map(int map_fd, void* key, void* value, int ops);
extern int attach_bpf(int pid, int extra_cost, int on_off);
extern int destroy_bpf();