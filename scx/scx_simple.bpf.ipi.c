/* SPDX-License-Identifier: GPL-2.0 */
/*
 * A simple scheduler.
 *
 * By default, it operates as a simple global weighted vtime scheduler and can
 * be switched to FIFO scheduling. It also demonstrates the following niceties.
 *
 * - Statistics tracking how many tasks are queued to local and global dsq's.
 * - Termination notification for userspace.
 *
 * While very simple, this scheduler should work reasonably well on CPUs with a
 * uniform L3 cache topology. While preemption is not implemented, the fact that
 * the scheduling queue is shared across all CPUs means that whatever is at the
 * front of the queue is likely to be executed fairly quickly given enough
 * number of CPUs. The FIFO scheduling mode may be beneficial to some workloads
 * but comes with the usual problems with FIFO scheduling where saturating
 * threads can easily drown out interactive ones.
 *
 * Copyright (c) 2022 Meta Platforms, Inc. and affiliates.
 * Copyright (c) 2022 Tejun Heo <tj@kernel.org>
 * Copyright (c) 2022 David Vernet <dvernet@meta.com>
 */
#include <scx/common.bpf.h>

char _license[] SEC("license") = "GPL";

const volatile bool fifo_sched = true;


// target pid is the parent pid of the applications
const volatile u32 target_pid;

// timeslice of each quantum, can be changed while loading the bpf program
const volatile u64 TIME_QUANTUM=20000; // my runtime will set this before loading

const volatile u64 TICK_QUANTUM = 5000;

const volatile u64 EXTRA_COST_TIME = 3000;

const volatile u64 NORMAL_QUANTUM = 200000;

const volatile u64 SWITCH_WAIT_TIME = 100000;

// SIM_NR_CORES: SIM_NR_CORES is used to do simulation; other cores are used to run normal application 
const volatile u64 NR_CORES;
const volatile u64 SIM_NR_CORES;

static u64 sp_traced_cnt = 0;

#define SHARED_DSQ 0
#define Q1_Q2 1

#ifdef FIRST_HALF_AS_SIM
    #define NON_SIM_CORE_EXPR (cpu >= SIM_NR_CORES)
    #define PIN_CORE_EXPR ((u32) pid % SIM_NR_CORES)
    #define SIM_CORE_START 0
    #define SIM_CORE_END SIM_NR_CORES
#else
    #define NON_SIM_CORE_EXPR (cpu < NR_CORES-SIM_NR_CORES)
    #define PIN_CORE_EXPR ((u32) pid % SIM_NR_CORES+(NR_CORES-SIM_NR_CORES))
    #define SIM_CORE_START (NR_CORES-SIM_NR_CORES)
    #define SIM_CORE_END NR_CORES
#endif

#define TASK_TRACED 0x00000008
struct trace_evnt{
    u32 type;
    __u64 ts;
    __u64 data;
};


struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, struct trace_evnt);
}trace_event_q SEC(".maps");



struct event {
    u64 vts;
    u32 pid;
    u32 type;
};

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 1024);
    __type(value, struct event);
} event_q SEC(".maps");


struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, u32);
} expired_event_q SEC(".maps");


struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, u64);
    __uint(max_entries, 1);
} vts SEC(".maps");

struct sched_stats{
    u64 err_early_end;
    u64 err_late_end;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, struct sched_stats);
    __uint(max_entries, 1);
} sched_stats SEC(".maps");

// the map to store the target PID
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, u32);
} target_pid_map SEC(".maps");

// not used
struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 1024);
    __type(value, u32);
} traced_q SEC(".maps");

struct pstate {
    u64 sim_state;
    u64 traced_time;
};
// store the state of all tasks
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, struct pstate);
} sim_proc_state SEC(".maps");

struct monitor_timer {
    struct bpf_timer timer;
};

// timer per core
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 48);
    __type(key, u32);
    __type(value, struct monitor_timer);
} monitor_timer SEC(".maps");

// round is a counter to count how many times we switch between Q1 and Q2
static u32 round;

// counts how many times the cpu has been continously fetch from DSQ1 or 2 instead of the OTHER_DSQ
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 48);
    __type(key, u32);
    __type(value, u32);
} cpu_count SEC(".maps");

#ifdef DEBUG_TIMER
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 48);
    __type(key, u32);
    __type(value, u32);
} cpu_tasking SEC(".maps");
#endif

// keep the pid in Q1
struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, u32);
} id_dsq_1 SEC(".maps");

// keep the pid in Q2
struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, u32);
} id_dsq_2 SEC(".maps");


u64 trace_start = 0;
u64 trace_end = 0;

struct per_thread_state {
    u64 halt_until;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, struct per_thread_state);
} thread_state_map SEC(".maps");

// to store the real entry time of specific syscalls, of a process
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, u64);
} syscall_entry_real_time_map SEC(".maps");

UEI_DEFINE(uei);

/*
 * Built-in DSQs such as SCX_DSQ_GLOBAL cannot be used as priority queues
 * (meaning, cannot be dispatched to with scx_bpf_dispatch_vtime()). We
 * therefore create a separate DSQ with ID 0 that we dispatch to and consume
 * from. If scx_simple only supported global FIFO scheduling, then we could
 * just use SCX_DSQ_GLOBAL.
 */

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__uint(key_size, sizeof(u32));
	__uint(value_size, sizeof(u64));
	__uint(max_entries, 2);			/* [local, global] */
} stats SEC(".maps");

static inline void inc_vts(u64 time){
    u32 index = 0;
    u64* vts_ptr = bpf_map_lookup_elem(&vts, &index);
    if(vts_ptr){
        *vts_ptr += time;
    }
}

static inline void new_timer(s32 cpu, u64 quantum){
    struct bpf_timer *timer;
    int key = (int) cpu;
    timer = bpf_map_lookup_elem(&monitor_timer, &key);
    if (!timer)
        return;
    bpf_timer_start(timer, quantum, BPF_F_TIMER_CPU_PIN);
}

static int monitor_timerfn(void *map, int *key, struct bpf_timer *timer)
{
    // key is cpu number
    s32 cpu = (s32)(*key);
    
    scx_bpf_kick_cpu(cpu, SCX_KICK_PREEMPT);
    return 0;
}

static void stat_inc(u32 idx)
{
	u64 *cnt_p = bpf_map_lookup_elem(&stats, &idx);
	if (cnt_p)
		(*cnt_p)++;
}

static inline bool vtime_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

// TODO: optimize, every p go through this
static inline bool target_pid_from_userland(struct task_struct* p) {
    u32 pid = p->pid;
    u32 index = 0;

    // the target pid is a parent pid
    struct task_struct *current = p;
    int depth = 0; // Add a counter for depth control
    #define MAX_PARENTS 100
    while (current->real_parent && depth < MAX_PARENTS) {
        current = current->real_parent;
		if(current == NULL){
			break;
		}
        if (current->pid == target_pid) {
            return true;
        }
        depth++; // Increment depth to ensure the loop is bounded
    }
    return false;
}

static inline void kick_all_cpu(){
    u64 start = bpf_ktime_get_ns();
    for(int i=SIM_CORE_START; i<SIM_CORE_END; i++){
        scx_bpf_kick_cpu(i, SCX_KICK_PREEMPT);
    }
    u64 end = bpf_ktime_get_ns();
    // bpf_printk("Kick all cpu time %lu\n", end-start);
}

s32 BPF_STRUCT_OPS(simple_select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
    bool is_idle = false;
    s32 cpu;
	cpu = scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, &is_idle);
    return cpu;
}

void BPF_STRUCT_OPS(simple_enqueue, struct task_struct *p, u64 enq_flags)
{
    bool is_target_pid = target_pid_from_userland(p);
    if(is_target_pid){
        scx_bpf_dispatch(p, Q1_Q2, SCX_SLICE_DFL, enq_flags);
        u32 index = (u32)p->pid;
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        if(state_ptr){
            if(state_ptr->sim_state == 0x02){
                u64 now = bpf_ktime_get_ns();
                u64 inc_time = now - state_ptr->traced_time;
                bpf_printk("pid %d, state %lu, time %lu\n", index, state_ptr->sim_state, inc_time);
                inc_vts(inc_time); 
                state_ptr->traced_time = 0;
                state_ptr->sim_state = 0;
                __sync_fetch_and_add(&sp_traced_cnt, -1);
            }
        }
    }else{
        scx_bpf_dispatch(p, SHARED_DSQ, SCX_SLICE_DFL, enq_flags);
    }
}

// #define TICK_QUANTUM 5000

void BPF_STRUCT_OPS(simple_dispatch, s32 cpu, struct task_struct *prev)
{
    if(sp_traced_cnt > 0){
        scx_bpf_consume(SHARED_DSQ);
		new_timer(cpu, TICK_QUANTUM);
		return;
    }

	int ret = scx_bpf_consume(Q1_Q2);
	if(ret == 0){
		scx_bpf_consume(SHARED_DSQ);
		new_timer(cpu, TICK_QUANTUM);
        return;
	}
    new_timer(cpu, TICK_QUANTUM);
	return;
}

void BPF_STRUCT_OPS(simple_running, struct task_struct *p)
{
    new_timer(p->thread_info.cpu, p->scx.slice);
    return;
}

void BPF_STRUCT_OPS(simple_stopping, struct task_struct *p, bool runnable)
{
    if(!runnable && p->__state & TASK_TRACED){
        __sync_fetch_and_add(&sp_traced_cnt, 1);
        u64 now = bpf_ktime_get_ns();
        kick_all_cpu();
        u32 index = (u32)p->pid;
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        if(state_ptr){
            if(state_ptr->sim_state == 0x02){
                __sync_fetch_and_add(&sp_traced_cnt, -1);
            }else{
                state_ptr->sim_state = 0x02;
                state_ptr->traced_time = now;
            }
        }
    }
}

void BPF_STRUCT_OPS(simple_enable, struct task_struct *p)
{
    bool is_target_pid = target_pid_from_userland(p);
    u32 index = (u32)p->pid;
    if(is_target_pid){
        struct pstate state;
        state.sim_state = 0;
        state.traced_time = 0;
        bpf_map_update_elem(&sim_proc_state, &index, &state, BPF_ANY);
    }
}

s32 BPF_STRUCT_OPS_SLEEPABLE(simple_init)
{   
     for(int i = 0; i < NR_CORES; i++){
        u32 key = i;
        struct bpf_timer *timer;
        timer = bpf_map_lookup_elem(&monitor_timer, &key);
        if (!timer)
            return -ESRCH;

        bpf_timer_init(timer, &monitor_timer, CLOCK_MONOTONIC);
        bpf_timer_set_callback(timer, monitor_timerfn);
    }

	scx_bpf_create_dsq(Q1_Q2, -1);
	return scx_bpf_create_dsq(SHARED_DSQ, -1);
}

void BPF_STRUCT_OPS(simple_exit, struct scx_exit_info *ei)
{
	UEI_RECORD(uei, ei);
}

SCX_OPS_DEFINE(simple_ops,
	       .select_cpu		= (void *)simple_select_cpu,
	       .enqueue			= (void *)simple_enqueue,
	       .dispatch		= (void *)simple_dispatch,
           .enable          = (void *)simple_enable,
	       .running			= (void *)simple_running,
	       .stopping		= (void *)simple_stopping,
	       .init			= (void *)simple_init,
	       .exit			= (void *)simple_exit,
		   .flags     = SCX_OPS_ENQ_LAST,
	       .name			= "simple");
