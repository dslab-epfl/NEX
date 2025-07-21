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
static u64 vtime_now;

#define SHARED_DSQ 0
#define Q1_Q2 1

#define NR_CORES 24
#define SIM_NR_CORES 16
#define NON_SIM_CORE_EXPR (cpu < NR_CORES-SIM_NR_CORES)

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, u64);
    __uint(max_entries, 1);
} vts SEC(".maps");


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

// store the state of all tasks
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, u64);
} sim_proc_state SEC(".maps");

struct monitor_timer {
    struct bpf_timer timer;
};

// timer per core
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, NR_CORES);
    __type(key, u32);
    __type(value, struct monitor_timer);
} monitor_timer SEC(".maps");

// round is a counter to count how many times we switch between Q1 and Q2
static u32 round;

// counts how many times the cpu has been continously fetch from DSQ1 or 2 instead of the OTHER_DSQ
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, NR_CORES);
    __type(key, u32);
    __type(value, u32);
} cpu_count SEC(".maps");

#ifdef DEBUG_TIMER
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, NR_CORES);
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

s32 BPF_STRUCT_OPS(simple_select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
	bool is_idle = false;
	s32 cpu;

	cpu = scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, &is_idle);
	if (is_idle) {
		stat_inc(0);	/* count local queueing */
		scx_bpf_dispatch(p, SCX_DSQ_LOCAL, TIME_QUANTUM, 0);
	}
	return cpu;
}

void BPF_STRUCT_OPS(simple_enqueue, struct task_struct *p, u64 enq_flags)
{
	stat_inc(1);	/* count global queueing */

	if (fifo_sched) {
		if(target_pid_from_userland(p)){
			scx_bpf_dispatch(p, Q1_Q2, TIME_QUANTUM, enq_flags);
		}else{
			scx_bpf_dispatch(p, SHARED_DSQ, NORMAL_QUANTUM, enq_flags);
		}
	} else {
		u64 vtime = p->scx.dsq_vtime;
		/*
		 * Limit the amount of budget that an idling task can accumulate
		 * to one slice.
		 */
		if (vtime_before(vtime, vtime_now - TIME_QUANTUM))
			vtime = vtime_now - TIME_QUANTUM;

		scx_bpf_dispatch_vtime(p, SHARED_DSQ, TIME_QUANTUM, vtime,
				       enq_flags);
	}
}

// #define TICK_QUANTUM 5000

void BPF_STRUCT_OPS(simple_dispatch, s32 cpu, struct task_struct *prev)
{
	if(NON_SIM_CORE_EXPR){
		scx_bpf_consume(SHARED_DSQ);
		new_timer(cpu, TICK_QUANTUM);
		return;
	}

	int ret = scx_bpf_consume(Q1_Q2);
	if(ret == 0){
		scx_bpf_consume(SHARED_DSQ);
		new_timer(cpu, TICK_QUANTUM);
	}
	return;
}

void BPF_STRUCT_OPS(simple_running, struct task_struct *p)
{
	if (fifo_sched){
    	new_timer(p->thread_info.cpu, p->scx.slice+EXTRA_COST_TIME);
		return;
	}

	/*
	 * Global vtime always progresses forward as tasks start executing. The
	 * test and update can be performed concurrently from multiple CPUs and
	 * thus racy. Any error should be contained and temporary. Let's just
	 * live with it.
	 */
	if (vtime_before(vtime_now, p->scx.dsq_vtime))
		vtime_now = p->scx.dsq_vtime;

    new_timer(p->thread_info.cpu, p->scx.slice+EXTRA_COST_TIME);
}

void BPF_STRUCT_OPS(simple_stopping, struct task_struct *p, bool runnable)
{
	if (fifo_sched){
		return;
	}

	/*
	 * Scale the execution time by the inverse of the weight and charge.
	 *
	 * Note that the default yield implementation yields by setting
	 * @p->scx.slice to zero and the following would treat the yielding task
	 * as if it has consumed all its slice. If this penalizes yielding tasks
	 * too much, determine the execution time by taking explicit timestamps
	 * instead of depending on @p->scx.slice.
	 */
	p->scx.dsq_vtime += (TIME_QUANTUM - p->scx.slice) * 100 / p->scx.weight;
}

void BPF_STRUCT_OPS(simple_enable, struct task_struct *p)
{
	p->scx.dsq_vtime = vtime_now;
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
	       .running			= (void *)simple_running,
	       .stopping		= (void *)simple_stopping,
	       .enable			= (void *)simple_enable,
	       .init			= (void *)simple_init,
	       .exit			= (void *)simple_exit,
		   .flags     = SCX_OPS_ENQ_LAST,
	       .name			= "simple");
