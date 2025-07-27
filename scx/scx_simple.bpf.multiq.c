#include <scx/common.bpf.h>


// please set this manually, due to bpf static analysis
#define MAX_CORES 48

// #define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(...) bpf_printk(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

// #define REAL_LOCK 1
#ifdef REAL_LOCK 
#define LOCK bpf_spin_lock(&epoch_lock)
#define UNLOCK bpf_spin_unlock(&epoch_lock)
#define ATOMIC_MINUS_ONE(x) x -= 1
#define ATOMIC_PLUS_ONE(x) x += 1
#define ATOMIC_SET_ZERO(x) x = 0
#define ATOMIC_READ(x) x
#else
#define LOCK
#define UNLOCK
#define ATOMIC_PLUS_X(x, y) __sync_fetch_and_add(&x, y)
#define ATOMIC_MINUS_ONE(x) __sync_fetch_and_add(&x, -1)
#define ATOMIC_PLUS_ONE(x) __sync_fetch_and_add(&x, 1)
#define ATOMIC_SET_ZERO(x) __sync_fetch_and_and(&x, 0)
#define ATOMIC_READ(x) __sync_fetch_and_add(&x, 0)
#endif


// #define PIN_SIM_CORE 1

char _license[] SEC("license") = "GPL";

// target pid is the parent pid of the applications
const volatile u32 target_pid;

// timeslice of each quantum, can be changed while loading the bpf program
const volatile u64 TIME_QUANTUM; // my runtime will set this before loading

static volatile u64 TIME_QUANTUM_TO_USE; 

static volatile u64 DYN_TIME_QUANTUM_TO_USE; 

const volatile u64 TICK_QUANTUM = 5000;

const volatile u64 EXTRA_COST_TIME = 2000;

const volatile u64 NORMAL_QUANTUM = 20000000;

const volatile bool DEFAULT_ON_OFF = 0;

const volatile bool EAGER_SYNC = 0;
const volatile u64 EAGER_SYNC_PERIOD = 0;

static u64 eager_last_sync_vts = 0;

// SIM_NR_CORES: SIM_NR_CORES is used to do simulation; other cores are used to run normal application 
const volatile u64 NR_CORES;
const volatile u64 SIM_NR_CORES;

const volatile u64 SIM_VIRT_CORES = 0;
// note SIM_NR_CORES must be <= SIM_VIRT_CORES when SIM_VIRT_CORES is not zero

static u64 pin_core_cnt[MAX_CORES];

// keep 1 counter to counter to count total enabled tasks;
// keep 2 counters to count separately how many tasks are enabled;
// keep 2 counters to count how many threads have stopped in each queue

// static u16 sp_traced_cnt = 0;
// static u16 sim_total_enabled;
// static u16 q1_total_enabled;
// static u16 q2_total_enabled;

static u64 all_in_one_counter = 0;

#define TRACED_CNT_MASK 0xFFFF000000000000
#define SIM_TOTAL_MASK 0x0000FFFF00000000
#define Q1_CNT_MASK 0x00000000FFFF0000
#define Q2_CNT_MASK 0x000000000000FFFF

#define TRACED_CNT_SHIFT 48 // traced_cnt: bits 48-63
#define TOTAL_CNT_SHIFT 32 // total_enabled: bits 32-47
#define Q1_CNT_SHIFT 16 // q1_total: bits 16-31
#define Q2_CNT_SHIFT 0  // q2_total: bits 0-15


static __inline void atomic_inc_cnt(uint16_t shift) {
    __sync_fetch_and_add(&all_in_one_counter, 1ULL << shift);
}

static __inline void atomic_dec_cnt(uint16_t shift) {
    __sync_fetch_and_sub(&all_in_one_counter, 1ULL << shift);
}

static __inline void atomic_inc_total_and_qcnt(uint16_t q_shift) {
    int64_t delta = (1ULL << TOTAL_CNT_SHIFT) + (1ULL << q_shift);
    __sync_fetch_and_add(&all_in_one_counter, delta);
}

static __inline void atomic_inc_qcnt(uint16_t q_shift) {
    __sync_fetch_and_add(&all_in_one_counter, 1ULL << q_shift);
}

static __inline void atomic_inc_f1_dec_f2(uint16_t f1_shift, uint16_t f2_shift) {
    int64_t delta = (1ULL << f1_shift) - (1ULL << f2_shift);
    __sync_fetch_and_add(&all_in_one_counter, delta);
}

static __inline void atomic_dec_f1_dec_f2(uint16_t f1_shift, uint16_t f2_shift) {
    int64_t delta = ((1ULL << f1_shift) + (1ULL << f2_shift));
    __sync_fetch_and_sub(&all_in_one_counter, delta);
}


// static __inline u16 get_cnt(u16 shift) {
//     return (all_in_one_counter >> shift) & 0xFFFF;
// }

// static __inline void atomic_dec_cnt_multi(uint16_t shift, uint64_t cnt) {
//     __sync_fetch_and_sub(&all_in_one_counter, cnt << shift);
// }

static bool on_off = 0; // off by default

// lock, to protect concurrent access to shared variables ( mainly counters ) 
private(SIM_PROC_LOCK) struct bpf_spin_lock epoch_lock;

// keep a counter to count number of traced threads


// Q1 and Q2 are two random numbers
// consume_q points to one of the queues
// other dsq is to hold all other tasks, why do we need other_dsq ? because cpus only call dispatch if no task in local and global queues,
// if other tasks by default goes into those two queues, all sim code will have to wait for a long time to run
static u32 consume_q;
static u32 next_consume_q;
static u32 nex_runtime_sync_counter = 0;
static u32 wait_nex_runtime = 0;

#define Q1 1
#define Q2 2

// every cpu has 2 queues <cpu>-Q1 and <cpu>-Q2
#define OTHER_DSQ_CONFLICT 98
#define OTHER_DSQ 99
#define SHARED_DSQ 100
#define START_Q 102

// SIM_NR_CORES: SIM_NR_CORES is used to do simulation; other cores are used to run normal application 
// #define NR_CORES 24
// #define SIM_NR_CORES 16

//interleaved core mapping
// #define HALF_CORE 24
//Virtual  Core ID: 0, 1, 2, 3, 4, 5, 6, 7 .. 23, 24, 25, 26, ............. 47
//Physical Core ID: 0, 2, 4, 6, 8, .......... 46,  1,  3,  5, ............. 47
// this mapping only works for two sockets
// #define VIRT_CPU_TO_PHY(cpu) ((cpu) <= HALF_CORE-1 ? (cpu)<<1 : (((cpu)-HALF_CORE)<<1)+1)
// #define PHY_CPU_TO_VIRT(cpu) (((cpu) & 0x1) == 0 ? (cpu) >> 1 : (((cpu)-1) >> 1) + HALF_CORE)

//Virtual  Core ID: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, ............. 47
//Physical Core ID: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, ............. 47

#define VIRT_CPU_TO_PHY(cpu) cpu 
#define PHY_CPU_TO_VIRT(cpu) cpu

#define FIRST_HALF_AS_SIM
#ifdef FIRST_HALF_AS_SIM
    #define NON_SIM_CORE_EXPR ( PHY_CPU_TO_VIRT(cpu) >= SIM_NR_CORES)
    #define PIN_CORE_EXPR ((u32) VIRT_CPU_TO_PHY((p->pid) % SIM_NR_CORES))
    #define SIM_CORE_START 0
    #define SIM_CORE_END SIM_NR_CORES
#else
    #define NON_SIM_CORE_EXPR (PHY_CPU_TO_VIRT(cpu) < NR_CORES-SIM_NR_CORES)
    #define PIN_CORE_EXPR ((u32) VIRT_CPU_TO_PHY((p->pid) % SIM_NR_CORES+(NR_CORES-SIM_NR_CORES)))
    #define SIM_CORE_START (NR_CORES-SIM_NR_CORES)
    #define SIM_CORE_END NR_CORES
#endif

struct custom_event{
    u32 type;
    u64 ts;
    u64 data;
};

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 1024);
    __type(value, struct custom_event);
}to_nex_runtime_event_q SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 1024);
    __type(value, struct custom_event);
}from_nex_runtime_event_q SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, u64);
} bpf_sched_ctrl SEC(".maps");
    

// stats for keep track of error 

static u64 total_over_diff = 0;
static u64 total_less_diff = 0;

/* vts is virtual timestamp, second entry to restore time after trace */ 
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, u64);
    __uint(max_entries, 2);
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

// not used
struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 1024);
    __type(value, u32);
} traced_q SEC(".maps");

/*
 store the state of all tasks
  state & 0XFF : 
    if 255 -> initial state; if 0 -> normal; 
    if 1 -> stopped and not runnable; 
    if 2 -> traced; 
    if 3 -> stopped and runnable (slice is up);
*/
struct pstate {
    u64 sim_state;
    // u64 compensate;
    u64 epoch_dur;
    u32 pin_cpu;
    u32 ctrl_msg;
    bool jailbreak;
    // first 32 bits is the wrap around counter for the CPU it's pinned on
    // the second 32 bits is used to detect if a round is finished
    u64 reversed_priority;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, struct pstate);
} sim_proc_state SEC(".maps");


struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, u64);
} err_q1 SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, u64);
} err_q2 SEC(".maps");

/* timer; stored per core */
struct monitor_timer {
    struct bpf_timer timer;
    u64 start;
    u64 end;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MAX_CORES);
    __type(key, u32);
    __type(value, struct monitor_timer);
} monitor_timer_arr SEC(".maps");

//store the ns3 events to resume the scheduler and ptrace_cont the process after ns3 simulation
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

// to store pid of the process to send ptrace_cont after vts becomes >= simulation time
struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 10240);
    __type(value, u32);
} expired_event_q SEC(".maps");

struct per_thread_state {
    u64 halt_until;
};

// to store the halt_until and other thread state information
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


u64 cpu_q1_cnt[MAX_CORES] = {0};
u64 cpu_q2_cnt[MAX_CORES] = {0};

#define LARGEST_U64 0xFFFFFFFFFFFFFFFF
#define MAX_PRIORITY 20000
#define RESCHEDULE_PENALTY 10
#define MIGRATE_THRESHOLD 10 // 1ms slack // threshold to migrate
#define MAX_USED_CREDITS_TRACK 100000 // 100ms window size

u32 cpu_sched_round_cnt[MAX_CORES] = {0};
int cpu_sched_used_credits[MAX_CORES] = {0};
int cpu_sched_total_credits[MAX_CORES] = {0};
bool cpu_sched_next_epoch_allowed[MAX_CORES] = {true};

static inline bool cpu_sched_allowed(u32 cpu){
    if(cpu < MAX_CORES){
        return cpu_sched_total_credits[cpu] > 0;
    }
    return true;
}

static inline void cpu_sched_stats_init(){
    for(int i=0; i<MAX_CORES; i++){
        cpu_sched_used_credits[i] = 0;
        cpu_sched_total_credits[i] = 0;
        cpu_sched_next_epoch_allowed[i] = true;
    }
}

static u32 track_rounds = 0;

// this happens when the scheduling is switching, no work is flying now
static void cpu_sched_set_stats_for_all(){
    // make sure this integer division devides.
    u64 inc_credits = 1;
    if(SIM_NR_CORES < SIM_VIRT_CORES){
        inc_credits = SIM_VIRT_CORES/SIM_NR_CORES;
    }
    track_rounds += 1;
    // physical cpu number, not virtual cpu number
    for(int cpu=0; cpu<MAX_CORES; cpu++){
        if(NON_SIM_CORE_EXPR){
            continue;
        }
        if(cpu_sched_total_credits[cpu] > 0){
            // reset total credits
            cpu_sched_total_credits[cpu] = inc_credits;
        }else{
            // otherwise, add the credits
            cpu_sched_total_credits[cpu] += inc_credits;
        }
        if(track_rounds >= MAX_USED_CREDITS_TRACK){
            // clear tracking for CPU usage
            cpu_sched_used_credits[cpu] = 0;
        }
    }
    track_rounds %= MAX_USED_CREDITS_TRACK;
}

static inline void cpu_sched_stats_inc_used(u32 pcpu, u64 credits){
    if(pcpu < MAX_CORES){
        cpu_sched_used_credits[pcpu] += credits;
    }
}

static inline void cpu_sched_stats_dec_total(u32 pcpu, u64 credits){
    if(pcpu < MAX_CORES){
        cpu_sched_total_credits[pcpu] -= credits;
    }
}

static inline int cpu_sched_stats_total(u32 pcpu){
    if(pcpu < MAX_CORES){
        return cpu_sched_total_credits[pcpu];
    }
    return 0;
}

static inline int cpu_sched_stats_used(u32 pcpu){
    if(pcpu < MAX_CORES){
        return cpu_sched_used_credits[pcpu];
    }
    return 0;
}

/* round is a counter to count how many times we switch between Q1 and Q2 */
static u32 round;
static u64 round64;

/* 
Not used;
Counts how many times the cpu has been continously fetch from DSQ1 or 2 instead of the OTHER_DSQ
*/ 
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MAX_CORES);
    __type(key, u32);
    __type(value, u32);
} cpu_count SEC(".maps");

UEI_DEFINE(uei);

#define TASK_RUNNING            0x00000000
#define TASK_INTERRUPTIBLE      0x00000001
#define TASK_UNINTERRUPTIBLE        0x00000002
#define __TASK_STOPPED          0x00000004
#define __TASK_TRACED           0x00000008
#define EXIT_DEAD           0x00000010
#define EXIT_ZOMBIE         0x00000020
#define EXIT_TRACE          (EXIT_ZOMBIE | EXIT_DEAD)
#define TASK_PARKED         0x00000040
#define TASK_DEAD           0x00000080
#define TASK_WAKEKILL           0x00000100
#define TASK_WAKING         0x00000200
#define TASK_NOLOAD         0x00000400
#define TASK_NEW            0x00000800
#define TASK_RTLOCK_WAIT        0x00001000
#define TASK_FREEZABLE          0x00002000
#define __TASK_FREEZABLE_UNSAFE        (0x00004000 * IS_ENABLED(CONFIG_LOCKDEP))
#define TASK_FROZEN         0x00008000
#define TASK_STATE_MAX          0x00010000
#define TASK_ANY            (TASK_STATE_MAX-1)
#define TASK_FREEZABLE_UNSAFE       (TASK_FREEZABLE | __TASK_FREEZABLE_UNSAFE)
#define TASK_KILLABLE           (TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED            (TASK_WAKEKILL | __TASK_STOPPED)
#define TASK_TRACED         __TASK_TRACED

/* increment the virtual timestamp in nanoseconds */
static inline void inc_vts(u64 time){
    if(time > 10000) return;
    u32 index = 0;
    u64* vts_ptr = bpf_map_lookup_elem(&vts, &index);
    if(vts_ptr){
        *vts_ptr += time;
        // DEBUG_PRINT("vts: %lu\n", *vts_ptr);
        // DEBUG_PRINT("vts: %lu\n", *vts_ptr);
    }
}

static inline u64 reset_vts(){
    u32 index = 0;
    u64* vts_ptr = bpf_map_lookup_elem(&vts, &index);
    if(vts_ptr){
        *vts_ptr = 0;
    }
}

static inline void dec_vts(u64 time){
    if(time > 10000) return;
    u32 index = 0;
    u64* vts_ptr = bpf_map_lookup_elem(&vts, &index);
    if(vts_ptr){
        if(*vts_ptr > time){
            *vts_ptr -= time;
        }
    }
}
static inline u64 read_vts(){
    u32 index = 0;
    u64 vts_val = 0;
    u64* vts_ptr = bpf_map_lookup_elem(&vts, &index);
    if(vts_ptr){
        vts_val = *vts_ptr;
    }
    return vts_val;
}

// SCX_KICK_PREEMPT or SCX_KICK_IDLE
// preempt all cpu now
static inline void kick_all_cpu(u64 flags){
    int pcpu=0;
    for(int i=SIM_CORE_START; i<SIM_CORE_END; i++){
        pcpu = VIRT_CPU_TO_PHY(i);
        scx_bpf_kick_cpu(pcpu, flags);
    }
}

/*
  Check if p is under the process tree of the target pid;
  if so, it's the target application
*/
static inline bool target_pid_from_userland_init(struct task_struct* p) {
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

static inline int target_pid_from_userland(struct task_struct* p) {
    u32 pid = p->pid;
    struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &pid);
    if(state_ptr == NULL){
        return 0;
    }

    if(state_ptr){
        if(state_ptr->jailbreak){
            return -1;
        }
    }
    return 1;
}

/*
    new_timer: start a new timer for a cpu;
    quantum is relative expiration time in nanoseconds from now;
    timer overwrites the previous timer
*/
static inline void new_timer(s32 cpu, u64 quantum, bool record){
    struct monitor_timer *ptr;
    int key = (int) cpu;
    ptr = bpf_map_lookup_elem(&monitor_timer_arr, &key);
    if (!ptr)
        return;
    // ptr->start = 0;
    // if(true || record){
    //     ptr->start = bpf_ktime_get_ns();
    // }
    if(on_off == 1 && SIM_VIRT_CORES != 0 && quantum == 0 && record){
        scx_bpf_kick_cpu(cpu, SCX_KICK_PREEMPT);
        return;
    }
    bpf_timer_start(&(ptr->timer), quantum, BPF_F_TIMER_CPU_PIN);
}

static inline u64 get_timer_start(s32 cpu){
    struct monitor_timer *ptr;
    int key = (int) cpu;
    ptr = bpf_map_lookup_elem(&monitor_timer_arr, &key);
    if (!ptr)
        return 0;
    return ptr->start;
}

static int monitor_timerfn(void *map, int *key, struct bpf_timer *timer)
{
    s32 cpu = (s32)(*key);

// #define DEBUG_TIMER
#ifdef DEBUG_TIMER
    u64 now = bpf_ktime_get_ns();
    u64 start = get_timer_start(cpu);
    if(start > 0){
        u64 elapsed = now - start;
        if(elapsed > (DYN_TIME_QUANTUM_TO_USE + EXTRA_COST_TIME + 5000)){
            u32 index = 0;
            struct sched_stats* ptr = bpf_map_lookup_elem(&sched_stats, &index);
            if(ptr){
                // this is wrong, should be per-round error
                ptr->err_late_end += elapsed - (DYN_TIME_QUANTUM_TO_USE + EXTRA_COST_TIME );
            }
        }
    }
#endif

    scx_bpf_kick_cpu(cpu, SCX_KICK_PREEMPT);
    return 0;
}

// Context structure to pass task_struct and min_cpu
struct ctx {
    struct task_struct *p;
    u32 *min_cpu_ptr;
    u64 *core_cnt;
};

// Callback function
static long callback(u32 index, void *ctx) {

        if(index < SIM_CORE_START){
            return 0;
        }

        if(index >= SIM_CORE_END){
            return 1;
        }
        
        u32 pcpu = VIRT_CPU_TO_PHY(index);

        if(pcpu > MAX_CORES){
            return 1;
        }

        struct ctx *args = ctx;  // Cast context to ctx struct
        struct task_struct *p = args->p;
        u32 *min_cpu_ptr = args->min_cpu_ptr;
        u32 min_cpu = *min_cpu_ptr;
        if(min_cpu > 1000){
            return 1;
        }
        u64 *pin_core_cnt = args->core_cnt;
        
        // Check if CPU is in the allowed mask
        if (bpf_cpumask_test_cpu(pcpu, p->cpus_ptr)) {
            if (min_cpu == 1000) {
                min_cpu = pcpu;  // Update the smallest CPU
            }
            if(min_cpu < NR_CORES){
                if(on_off == 1 && SIM_VIRT_CORES != 0){
                    int pcpu_busy_indicator = cpu_sched_used_credits[pcpu] + pin_core_cnt[pcpu]*MIGRATE_THRESHOLD;
                    int min_cpu_busy_indicator = cpu_sched_used_credits[min_cpu] + + pin_core_cnt[min_cpu]*MIGRATE_THRESHOLD;
                    if(min_cpu_busy_indicator > pcpu_busy_indicator + MIGRATE_THRESHOLD) 
                    {
                        min_cpu = pcpu;
                    }
                }else{
                    if(pin_core_cnt[pcpu] < pin_core_cnt[min_cpu]
                    ){
                        min_cpu = pcpu;
                    }
                }
                
            }
        }
        *min_cpu_ptr = min_cpu;
        return 0;
}



/*
    Calculate the pinned cpu for a task
*/
static u32 calc_cpu_balanced(struct task_struct *p, int old_cpu) {
    u32 min_cpu = 1000;
    if(bpf_cpumask_test_cpu(old_cpu, p->cpus_ptr)){
        min_cpu = old_cpu;
    }
    struct ctx context = {
        .p = p,
        .min_cpu_ptr = &min_cpu,
        .core_cnt = pin_core_cnt,
    };
    bpf_loop(MAX_CORES, callback, &context, 0);

    // After loop, process the result
    if (min_cpu>=0 && min_cpu < NR_CORES) {
        ATOMIC_PLUS_ONE(pin_core_cnt[min_cpu]);
        if (old_cpu >= 0 && old_cpu < NR_CORES) {
            ATOMIC_MINUS_ONE(pin_core_cnt[old_cpu]);
        }
    } else {
        // DEBUG_PRINT("No valid CPU found for PID %d\n", p->pid);
    }
    return min_cpu;
}

static bool calc_conflict(struct task_struct* p){
    struct cpumask* cpu_mask = (struct cpumask*)p->cpus_ptr;
    int pcpu = 0;
    for(u32 ci = 0; ci < MAX_CORES; ci++){
        if(ci >= SIM_CORE_START && ci < SIM_CORE_END){
            continue;
        }
        pcpu = VIRT_CPU_TO_PHY(ci);
        if(bpf_cpumask_test_cpu(pcpu, cpu_mask)){
            return false;
        }
    }
    return true;
}

/*
    Enqueue a task to a queue
*/
static inline void enqueue_sim_task(struct task_struct* p, u32 Q1_or_Q2, u64 quantum, u64 enq_flags, u32 cpu){
    if(on_off == 0){
        scx_bpf_dispatch(p, SHARED_DSQ+Q1_or_Q2-1, quantum, enq_flags);
    }else{
        if(SIM_VIRT_CORES == 0){
            scx_bpf_dispatch(p, cpu*2+START_Q+Q1_or_Q2-1, quantum, enq_flags);
        }else{
            // on == 1 and SIM_VIRT_CORES is on as well
            // smaller priority number runs earlier

            u64 reversed_priority = 0;
            u32 index = (u32)p->pid;
            struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
            if(state_ptr){
                reversed_priority = state_ptr->reversed_priority;
            }
            // larger is ordered earlier
            scx_bpf_dispatch_vtime(p, cpu*2+START_Q+Q1_or_Q2-1, quantum, reversed_priority, enq_flags);
        }
    }
}

/*
    select a cpu for a task, but it almost has no effect, except we can wake up the idle cpu early.
*/
s32 BPF_STRUCT_OPS(simple_select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
    bool is_idle = false;
    int is_target_pid = target_pid_from_userland(p);
    if(is_target_pid == 1){
        u32 index = (u32)p->pid;
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        u32 cpu = 0;
        if(state_ptr){
            cpu = state_ptr->pin_cpu;
        }
        // u32 cpu = calc_cpu(p);
        scx_bpf_kick_cpu(cpu, SCX_KICK_IDLE);
        return cpu;
    }
    s32 cpu;
	cpu = scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, &is_idle);
    return cpu;
}



/*
    enqueue a task to a queue
*/
void BPF_STRUCT_OPS(simple_enqueue, struct task_struct *p, u64 enq_flags)
{
    int is_target_pid = target_pid_from_userland(p);
    if(is_target_pid == 1){

        u32 index = (u32)p->pid;
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        u64 sim_state = 0;
        u32 pin_cpu = 0;
        u64 quantum = DYN_TIME_QUANTUM_TO_USE + EXTRA_COST_TIME;
        u32 priority = 0;
        if(state_ptr){
            sim_state = state_ptr->sim_state;
            pin_cpu = state_ptr->pin_cpu;
            if(state_ptr->epoch_dur > 0){
                quantum = state_ptr->epoch_dur + EXTRA_COST_TIME;
            }
            priority = state_ptr->reversed_priority & 0xFFFFFFFF;
        }
        u32 run_state = sim_state & 0xFF;
        u32 prev_q = (sim_state & 0xFF00) >> 8;
        u32 enq_q = 0;
        u32 consume_q_copy = consume_q;

        if(run_state == 0){
            bpf_printk("WARN: enqueue status is 0; Target PID %d, prev_enq %d\n", p->pid, prev_q);
        }

        LOCK;
        if((run_state > 0 && run_state < 3) || run_state == 255){
            // enq_q = Q2 + Q1 - consume_q_copy;
            // DON'T use the above; this is important;
            // because when the small slice scheduling is off, 
            // the following still allows the threads to run quickly.
            enq_q = consume_q_copy;
            // DEBUG_PRINT("Enqueue, Target PID %d, state %d, on_off %d\n", p->pid, run_state, on_off);
            if(enq_q == Q1){
                atomic_inc_total_and_qcnt(Q1_CNT_SHIFT);
            }else if(enq_q == Q2){
                atomic_inc_total_and_qcnt(Q2_CNT_SHIFT);
            }
        }else{
            if(prev_q == Q1){
                enq_q = Q2;
                if(run_state == 0){
                    atomic_inc_f1_dec_f2(Q2_CNT_SHIFT, Q1_CNT_SHIFT);
                }else{
                    atomic_inc_qcnt(Q2_CNT_SHIFT);
                }
            }else{
                enq_q = Q1;
                if(run_state == 0){
                    atomic_inc_f1_dec_f2(Q1_CNT_SHIFT, Q2_CNT_SHIFT);
                }else{
                    atomic_inc_qcnt(Q1_CNT_SHIFT);
                }   
            }
        }
        UNLOCK;

        // the task may not be able to run on the old cpu
        if(!bpf_cpumask_test_cpu(pin_cpu, p->cpus_ptr) 
           || (on_off == 1 && SIM_VIRT_CORES != 0 && MAX_PRIORITY == priority)
        ){
            // DEBUG_PRINT("<enqueue>CPU %d is not in the cpumask of PID %d\n", p->pid, pin_cpu);
            u32 old_cpu = pin_cpu;
            pin_cpu = calc_cpu_balanced(p, pin_cpu);
            scx_bpf_kick_cpu(pin_cpu, SCX_KICK_IDLE);
            // if(old_cpu != pin_cpu){
            //    DEBUG_PRINT("move from CPU %d to new CPU %d for PID %d\n", old_cpu, pin_cpu, p->pid);
            // }
        }

        sim_state = round << 16 | enq_q << 8 | 0x00;

        if(state_ptr){
            state_ptr->sim_state = sim_state;
            state_ptr->pin_cpu = pin_cpu;
        }

        if(run_state != 2){
            if(enq_q == Q1){
                DEBUG_PRINT("Enqueue, target PID %d enter Q1, Quantum %d\n", p->pid, quantum);
                enqueue_sim_task(p, Q1, quantum, enq_flags, pin_cpu);
            }else if(enq_q == Q2){
                DEBUG_PRINT("Enqueue, target PID %d enter Q2, Quantum %d\n", p->pid, quantum);
                enqueue_sim_task(p, Q2, quantum, enq_flags, pin_cpu);
            }
        }else{
            // TRACED
            u8 dec_traced_cnt = 1;
            u8 normal_enqueue = 1;
            u32 ctrl_msg = 0;
            if(state_ptr){
                ctrl_msg = state_ptr->ctrl_msg;
                state_ptr->ctrl_msg = 0;
            }

            if(ctrl_msg!=0){
                // this is a ctrl_msg
                if(ctrl_msg == 0x1000){
                    // switch on 
                    on_off = 1;
                    TIME_QUANTUM_TO_USE = TIME_QUANTUM;
                    DYN_TIME_QUANTUM_TO_USE = TIME_QUANTUM;
                    u32 _index = 0;
                    u64 _state = 1;
                    bpf_map_update_elem(&bpf_sched_ctrl, &_index, &_state, BPF_ANY);
                }else if(ctrl_msg == 0x2000){
                    // switch off
                    on_off = 0;
                    TIME_QUANTUM_TO_USE = NORMAL_QUANTUM;
                    DYN_TIME_QUANTUM_TO_USE = NORMAL_QUANTUM;
                    u32 _index = 0;
                    u64 _state = 0;
                    bpf_map_update_elem(&bpf_sched_ctrl, &_index, &_state, BPF_ANY);
                }else if((ctrl_msg & 0xFF00) == 0x3000){
                    // marker to virtually speedup a code segment start
                    // the epoch of the corresponding thread should be enlarged from now on, untill virtual speedup is off
                    // the percentage to speedup is the last 2 digits of the ctrl message
                    u16 speedup = ctrl_msg & 0x00FF;
                    #define SCALE_FACTOR 10000
                    u64 new_epoch_dur = (u64)((TIME_QUANTUM * 100 * SCALE_FACTOR) / (100 - speedup));
                    new_epoch_dur = new_epoch_dur / SCALE_FACTOR;
                    if(state_ptr){
                        state_ptr->epoch_dur = new_epoch_dur;
                    }
                    bpf_printk("Ctrl msg, msg is %x, new_epoch_dur %ld\n", ctrl_msg, new_epoch_dur);
                    p->scx.slice = new_epoch_dur+EXTRA_COST_TIME;
                }else if(ctrl_msg == 0x4000){
                    // marker to virtually speedup a code segment end
                    // the epoch of the corresponding thread should be restored
                    if(state_ptr){
                        state_ptr->epoch_dur = 0;
                    }
                    p->scx.slice = DYN_TIME_QUANTUM_TO_USE+EXTRA_COST_TIME;
                }else if(ctrl_msg == 0x5000){
                    // marker to jail break current thread
                    // we won't decrease the traced cnt for the whole simulation
                    // all application threads are stopped
                    // but we jail break this thread who triggered this event to allow continue
                    // later, this thread will go to jail again at ctrl_msg = 0x6000
                    if(state_ptr){
                        state_ptr->jailbreak = true;
                    }
                    dec_traced_cnt = 0;
                    normal_enqueue = 0;
                }else if(ctrl_msg == 0x6000){
                    // the code will never enter here because the thread has jail broken already
                }else if(ctrl_msg == 0x7000){
                    // marker to jail break current thread
                    // but other threads can still run
                    if(state_ptr){
                        state_ptr->jailbreak = true;
                    }
                    dec_traced_cnt = 1;
                    normal_enqueue = 0;
                }else if(ctrl_msg == 0x8000){
                    // the code will never enter here because the thread has jail broken already
                }
                bpf_printk("Ctrl msg pid %d, msg is %x, quantum %ld\n", p->pid, ctrl_msg, DYN_TIME_QUANTUM_TO_USE);
            }

            dec_vts(TIME_QUANTUM_TO_USE);
            if(normal_enqueue){
                if(enq_q == Q1){
                    DEBUG_PRINT("Enqueue, target PID %d enter Q1, Quantum %d\n", p->pid, quantum);
                    enqueue_sim_task(p, Q1, quantum, enq_flags, pin_cpu);
                }else if(enq_q == Q2){
                    DEBUG_PRINT("Enqueue, target PID %d enter Q2, Quantum %d\n", p->pid, quantum);
                    enqueue_sim_task(p, Q2, quantum, enq_flags, pin_cpu);
                }
            }else{
                // revert queue cnt
                if(enq_q == Q1){
                    atomic_dec_f1_dec_f2(TOTAL_CNT_SHIFT, Q1_CNT_SHIFT);
                }else if(enq_q == Q2){
                    atomic_dec_f1_dec_f2(TOTAL_CNT_SHIFT, Q2_CNT_SHIFT);
                }
                scx_bpf_dispatch(p, OTHER_DSQ_CONFLICT, NORMAL_QUANTUM+EXTRA_COST_TIME, enq_flags);
            }
            
            if(dec_traced_cnt > 0){
                kick_all_cpu(SCX_KICK_IDLE);
                atomic_dec_cnt(TRACED_CNT_SHIFT);
            }

            DEBUG_PRINT("Enqueue, target PID %d from traced, consume_q %d\n", p->pid, enq_q);
        }

        return;
    }

    // jailbreak
    if(is_target_pid == -1){
        u32 index = (u32)p->pid;
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        u64 sim_state = 0;
        if(state_ptr){
            sim_state = state_ptr->sim_state;
        }
        u32 run_state = sim_state & 0xFF;
        if(run_state == 2){
            u32 ctrl_msg = 0;
            if(state_ptr){
                ctrl_msg = state_ptr->ctrl_msg;
                state_ptr->ctrl_msg = 0;
            }
            if(ctrl_msg == 0x6000 || ctrl_msg == 0x8000){
                // mark to remove jail break for the current thread 
                u32 pin_cpu = 0;
                u64 quantum = DYN_TIME_QUANTUM_TO_USE + EXTRA_COST_TIME;
                if(state_ptr){
                    sim_state = state_ptr->sim_state;
                    pin_cpu = state_ptr->pin_cpu;
                    if(state_ptr->epoch_dur > 0){
                        quantum = state_ptr->epoch_dur + EXTRA_COST_TIME;
                    }
                }
            
                u32 enq_q = consume_q;
                sim_state = enq_q << 8 | 0x00;
                // the task may not be able to run on the old cpu
                if(!bpf_cpumask_test_cpu(pin_cpu, p->cpus_ptr)){
                    pin_cpu = calc_cpu_balanced(p, pin_cpu);
                    scx_bpf_kick_cpu(pin_cpu, SCX_KICK_IDLE);
                }

                if(enq_q == Q1){
                    atomic_inc_total_and_qcnt(Q1_CNT_SHIFT);
                    DEBUG_PRINT("Enqueue, target PID %d enter Q1, Quantum %d\n", p->pid, quantum);
                    enqueue_sim_task(p, Q1, quantum, enq_flags, pin_cpu);
                }else if(enq_q == Q2){
                    atomic_inc_total_and_qcnt(Q2_CNT_SHIFT);
                    DEBUG_PRINT("Enqueue, target PID %d enter Q2, Quantum %d\n", p->pid, quantum);
                    enqueue_sim_task(p, Q2, quantum, enq_flags, pin_cpu);
                }
               
                DEBUG_PRINT("Ctrl msg pid %d, msg is %x, quantum %ld\n", p->pid, ctrl_msg, DYN_TIME_QUANTUM_TO_USE);

                if(state_ptr){
                    state_ptr->sim_state = sim_state;
                    state_ptr->pin_cpu = pin_cpu;
                    state_ptr->jailbreak = false;
                }
                
                if(ctrl_msg == 0x6000){
                    LOCK;
                    atomic_dec_cnt(TRACED_CNT_SHIFT);
                    UNLOCK;
                }

                kick_all_cpu(SCX_KICK_IDLE);

                return;    
            }else{
                sim_state = 0x00;
                if(state_ptr){
                    state_ptr->sim_state = sim_state;
                }

                if(ctrl_msg != 0){
                    // TRACED State doesn't really mean it has a ctrl message, it's not precise here
                    // So if ctrl_msg is 0, we won't error.
                    bpf_printk("ERROR: jail break pid %d, but not 0x6000 or 0x8000 ctrl msg, %x\n", p->pid, ctrl_msg);
                }
            }
        }
    }

    if(is_target_pid == -1 || calc_conflict(p)){
        scx_bpf_dispatch(p, OTHER_DSQ_CONFLICT, NORMAL_QUANTUM+EXTRA_COST_TIME, enq_flags);
    }else{
        scx_bpf_dispatch(p, OTHER_DSQ, NORMAL_QUANTUM+EXTRA_COST_TIME, enq_flags);
    }
}


// idle start keeps the timestamp when scheduling is not active
u64 switch_start_time = 0;
const volatile u64 SWITCH_WAIT_TIME = 100000;

void BPF_STRUCT_OPS(simple_dispatch, s32 cpu, struct task_struct *prev)
{

#define KICK_ME_AND_RETURN \
        new_timer(cpu, TICK_QUANTUM, false); \
        return; \
    
    if(NON_SIM_CORE_EXPR){
        int ret = scx_bpf_consume(OTHER_DSQ_CONFLICT);
		ret = ret | scx_bpf_consume(OTHER_DSQ);
        if(ret == 0){
            KICK_ME_AND_RETURN;
        }
    }

    u32 consume_q_copy = consume_q;
    if(on_off == 0){
        scx_bpf_consume(cpu*2+START_Q+Q1+Q2-consume_q_copy-1);
        scx_bpf_consume(cpu*2+START_Q+consume_q_copy-1);
        u32 dsq = SHARED_DSQ+consume_q_copy-1;
        int ret = scx_bpf_consume(dsq);
        if(ret == 0){
            ret = scx_bpf_consume(OTHER_DSQ_CONFLICT);
            // if(ret == 0){
            //     // no work
            //     KICK_ME_AND_RETURN;
            // }
        }else{
            return;
        }
    }else{
        // for savety, consume the shared queue first
        scx_bpf_consume(SHARED_DSQ+Q1+Q2-consume_q_copy-1);
        scx_bpf_consume(SHARED_DSQ+consume_q_copy-1);
        u32 dsq = cpu*2+START_Q+consume_q_copy-1;
        int ret = scx_bpf_consume(dsq);
        if(ret == 0){
            ret = scx_bpf_consume(OTHER_DSQ_CONFLICT);
            // if(ret == 0){
            //     // no work
            //     KICK_ME_AND_RETURN;
            // }
        }else{
            return;
        }
    }
    
    // only enters here if the Q1 or Q2 is empty

    // all counters are in a consistent view
    u16 sp_traced_cnt = (all_in_one_counter >> TRACED_CNT_SHIFT) & 0xFFFF;
    u16 sim_total_enabled = (all_in_one_counter >> TOTAL_CNT_SHIFT) & 0xFFFF;
    u16 q1_total_enabled = (all_in_one_counter >> Q1_CNT_SHIFT) & 0xFFFF;
    u16 q2_total_enabled = (all_in_one_counter >> Q2_CNT_SHIFT) & 0xFFFF;

    // guard, if sp_trace_cnt > 0, => someone is being traced, and stopped, 
    // guard, if sim_total_enabled is 0 => no task is enabled, hence don't start switching queue.
    if(sp_traced_cnt > 0 || sim_total_enabled == 0){
        DEBUG_PRINT("Traced cnt %d, sim_total_enabled %d\n", sp_traced_cnt, sim_total_enabled);
        KICK_ME_AND_RETURN;
    }

    if(on_off == 1 && sim_total_enabled != q1_total_enabled + q2_total_enabled){
        if(sim_total_enabled < q1_total_enabled + q2_total_enabled){
            bpf_printk("WARN: total enabled %d < q1 %d + q2 %d\n", sim_total_enabled, q1_total_enabled, q2_total_enabled);
        }
        KICK_ME_AND_RETURN;
    }

    if(on_off == 1 && EAGER_SYNC == 1 && ATOMIC_READ(wait_nex_runtime) > 0){
        struct custom_event evnt;
        int ret = bpf_map_pop_elem(&from_nex_runtime_event_q, &evnt);
        if(ret == 0){
            // bpf_printk("Popping from nex_runtime_event_q (vts: %ld, data: %ld: cur_vts %ld)\n", evnt.ts, evnt.data, read_vts());
            // increase the sync counter
            ATOMIC_PLUS_ONE(nex_runtime_sync_counter);
        }            
    }
    
    bpf_spin_lock(&epoch_lock);
    if(on_off == 1 && EAGER_SYNC == 1 && ATOMIC_READ(wait_nex_runtime) > 0){
        if(ATOMIC_READ(nex_runtime_sync_counter) > 0){
            ATOMIC_MINUS_ONE(nex_runtime_sync_counter);
            ATOMIC_MINUS_ONE(wait_nex_runtime);
            consume_q = next_consume_q;
        }
        goto NO_SWITCH;
    }else if( 
        (
            (consume_q == Q1 && q1_total_enabled == 0 && sim_total_enabled > 0)||
            (consume_q == Q2 && q2_total_enabled == 0 && sim_total_enabled > 0)
        )
    ){
        int inc_time = 0;
        if(consume_q == Q1){
            /*
                want to ensure that the enabled queue of next round is at least the current 
                total enabled, s.t. to wait for slow cores to enqueue all known tasks
            */
            if(sim_total_enabled <= q2_total_enabled && sim_total_enabled > 0 ){
				round += 1;
                round64 += 1;
                inc_time = 1;
                next_consume_q = Q2;
				
            }else{
                goto NO_SWITCH;
            }
        }else{
            if( sim_total_enabled <= q1_total_enabled && sim_total_enabled > 0){
	 	        round += 1;
                round64 += 1;
                inc_time = 1;
                next_consume_q = Q1;
            }else{
                goto NO_SWITCH;
            }
        }

        if(SIM_VIRT_CORES != 0){
            cpu_sched_set_stats_for_all();
        }
        if(on_off == 1 && EAGER_SYNC == 1){
            ATOMIC_PLUS_ONE(wait_nex_runtime);
        }else{
            consume_q = next_consume_q;
        }

        bpf_spin_unlock(&epoch_lock);
        
        if(inc_time==1){
    
            inc_vts(DYN_TIME_QUANTUM_TO_USE);
            DYN_TIME_QUANTUM_TO_USE = TIME_QUANTUM_TO_USE;

            if(on_off == 1 && EAGER_SYNC == 1){
                uint64_t _vts = read_vts();
                if(_vts >= eager_last_sync_vts+EAGER_SYNC_PERIOD){
                    eager_last_sync_vts = _vts;
                    // DEBUG_PRINT("Pushing to nex_kernel_event_q, vts %ld, eager_last_sync_vts %ld, period %d; wait cnt %d\n", _vts, eager_last_sync_vts, EAGER_SYNC_PERIOD, wait_nex_runtime);
                    struct custom_event evnt;
                    evnt.type = 0;
                    evnt.ts = _vts;
                    evnt.data = sim_total_enabled;
                    bpf_map_push_elem(&to_nex_runtime_event_q, &evnt, BPF_ANY);
                }else{
                    bpf_spin_lock(&epoch_lock);
                    ATOMIC_SET_ZERO(wait_nex_runtime);
                    consume_q = next_consume_q;
                    bpf_spin_unlock(&epoch_lock);
                }
            }
        }
    }else{
NO_SWITCH:
        bpf_spin_unlock(&epoch_lock);
    }
    KICK_ME_AND_RETURN;
}

void BPF_STRUCT_OPS(simple_running, struct task_struct *p)
{
// #define PRINT_RUNNING
#ifdef PRINT_RUNNING
    int is_target_pid = target_pid_from_userland(p);
    s32 cpu = p->thread_info.cpu;
    if(is_target_pid == 1){
        u32 index = (u32)p->pid;
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        u64 sim_state = 0;
        if(state_ptr){
            sim_state = state_ptr->sim_state;
        }
        u32 enq_q = (sim_state & 0xFF00)>>8;
        if((sim_state & 0xFF) != 0){
            bpf_printk("WARN: no enqueue; (%lu)(@cpu%d, %d, prev state %d) Target PID %d is running slice %d\n", bpf_ktime_get_ns()/1000, cpu, enq_q, sim_state & 0xFF, p->pid, p->scx.slice);
        }else{
            DEBUG_PRINT("(%lu)(@cpu%d, %d, prev state %d) Target PID %d is running slice %d\n", bpf_ktime_get_ns()/1000, cpu, enq_q, sim_state, p->pid, p->scx.slice);
        }
        new_timer(p->thread_info.cpu, p->scx.slice, true);
    }else{
        new_timer(p->thread_info.cpu, p->scx.slice, false);
    }
#else
    int is_target_pid = target_pid_from_userland(p);
    s32 cpu = p->thread_info.cpu;
    if(is_target_pid == 1){
        if(on_off == 1){
            if(p->scx.slice == SCX_SLICE_DFL){
                goto NO_TIMER;
            }
            if(p->scx.slice > 200*(DYN_TIME_QUANTUM_TO_USE+EXTRA_COST_TIME)){
                // bpf_printk("WARN: slice %d > quantum %d\n", p->scx.slice, DYN_TIME_QUANTUM_TO_USE+EXTRA_COST_TIME);
                p->scx.slice = DYN_TIME_QUANTUM_TO_USE+EXTRA_COST_TIME;
            }

            if(SIM_VIRT_CORES != 0){
                u32 index = (u32)p->pid;
                struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
                u64 reversed_priority = 0;
                if(state_ptr){
                    reversed_priority = state_ptr->reversed_priority;
                    if(cpu < MAX_CORES && cpu >= 0){
                        if((reversed_priority >> 32) > cpu_sched_round_cnt[cpu]){
                            // bpf_printk("WARN: reversed_priority %ld > round_cnt %ld, likely because of migrate\n", reversed_priority >> 32, cpu_sched_round_cnt[cpu]);
                            state_ptr->reversed_priority = ((u64)cpu_sched_round_cnt[cpu] << 32) | MAX_PRIORITY;
                        }
                    }
                }

                if(!cpu_sched_allowed(cpu)){
                    // bpf_printk("CPU %d is not allowed to run now (total %ld, used %ld)\n", cpu, cpu_sched_stats_total(cpu), cpu_sched_stats_used(cpu));
                    p->scx.slice = 0;
                }else{
                    // decrease the reversed_priority
                    if(state_ptr){
                        if(cpu < MAX_CORES && cpu >= 0){
                            if((reversed_priority & 0xFFFFFFFF) == 0){
                                cpu_sched_round_cnt[cpu] += 1;
                                reversed_priority = ((u64)cpu_sched_round_cnt[cpu] << 32) | MAX_PRIORITY;
                                cpu_sched_stats_dec_total(cpu, RESCHEDULE_PENALTY-1);
                                // DEBUG_PRINT("CPU %d run %d (priority %ld)\n", cpu, p->pid, reversed_priority & 0xFFFFFFFF);
                            }else{
                                // higher priority
                                reversed_priority -= 1;
                            }
                            state_ptr->reversed_priority = reversed_priority;
                            // if((reversed_priority & 0xFFF)==0){
                            //     DEBUG_PRINT("CPU %d run %d (priority %ld)\n", cpu, p->pid, reversed_priority & 0xFFFFFFFF);
                            // }
                        }
                    }
                    cpu_sched_stats_inc_used(cpu, 1);
                    cpu_sched_stats_dec_total(cpu, 1);
                }
            }
        }
        
        new_timer(cpu, p->scx.slice, true);
    }else{
        new_timer(cpu, p->scx.slice, false);
    }
NO_TIMER:
#endif
    return;
}


void BPF_STRUCT_OPS(simple_stopping, struct task_struct *p, bool runnable)
{

    int is_target_pid = target_pid_from_userland(p);
    u32 index = (u32)p->pid;
    if(is_target_pid == 1){
        // multiple process can be in the task_traced in current quantum
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        u64 sim_state = 0;
        if(state_ptr){
            sim_state = state_ptr->sim_state;
        }
        u32 enq_q = (sim_state & 0xFF00)>>8;
        u32 run_state = sim_state & 0xFF;
        
        if( run_state > 0 && run_state < 3 ){
            return;
        }

        if( run_state == 3 ){ 
            if(!runnable){
                if(p->__state & TASK_TRACED){
                    // uint64_t vts = read_vts();
                    // DEBUG_PRINT("(@%lu)Target PID %d is stopping state (prev state %d, round %d, slice %d): R %d state %d U %d I %d S %d T %d D %d\n", vts, p->pid, sim_state & 0x00FF, (sim_state & 0xFFFF0000)>>16, p->scx.slice, runnable, p->__state, p->__state & TASK_UNINTERRUPTIBLE, p->__state & TASK_INTERRUPTIBLE, p->__state & TASK_STOPPED, p->__state & TASK_TRACED, p->__state & TASK_DEAD);
                    sim_state = (sim_state & 0xFFFFFF00) | 0x02;
                    atomic_inc_f1_dec_f2(TRACED_CNT_SHIFT, TOTAL_CNT_SHIFT);
                    if(on_off == 0){
                        // after which no threads moves, unless the traced thread returns
                        kick_all_cpu(SCX_KICK_PREEMPT);
                    }
                }else{
                    atomic_dec_cnt(TOTAL_CNT_SHIFT);
                    sim_state = (sim_state & 0xFFFFFF00) | 0x01;

                    // put to lowest priority.
                    if(SIM_VIRT_CORES != 0 && state_ptr){
                        u32 cpu = p->thread_info.cpu;
                        if(cpu < MAX_CORES && cpu >= 0){
                            state_ptr->reversed_priority = ((u64)(cpu_sched_round_cnt[cpu]) << 32) | MAX_PRIORITY;
                            cpu_sched_stats_dec_total(cpu, RESCHEDULE_PENALTY);
                        }
                    }
                }
                if(state_ptr){
                   state_ptr->sim_state = sim_state;
                }   
            }
            DEBUG_PRINT("<Stopping 4>(%lu)Target PID %d is stopping state (prev state %d, round %d, slice %d): R %d state %d U %d I %d S %d T %d D %d\n", bpf_ktime_get_ns()/1000, p->pid, sim_state & 0x00FF, (sim_state & 0xFFFF0000)>>16, p->scx.slice, runnable, p->__state, p->__state & TASK_UNINTERRUPTIBLE, p->__state & TASK_INTERRUPTIBLE, p->__state & TASK_STOPPED, p->__state & TASK_TRACED, p->__state & TASK_DEAD);
            return;
        }

        if(!runnable){
            // traced and not already recorded
            if(p->__state & TASK_TRACED){
                #define TRACED 2
                // uint64_t vts = read_vts();
                // DEBUG_PRINT("(vts@%lu)Target PID %d is stopping state (prev state %d, round %d, slice %d): R %d state %d U %d I %d S %d T %d D %d\n", vts, p->pid, sim_state & 0x00FF, (sim_state & 0xFFFF0000)>>16, p->scx.slice, runnable, p->__state, p->__state & TASK_UNINTERRUPTIBLE, p->__state & TASK_INTERRUPTIBLE, p->__state & TASK_STOPPED, p->__state & TASK_TRACED, p->__state & TASK_DEAD);
                sim_state = (sim_state & 0xFFFFFF00) | 0x02;
                atomic_inc_cnt(TRACED_CNT_SHIFT);
                if(on_off == 0){
                    kick_all_cpu(SCX_KICK_PREEMPT);
                }    
            }else{
                #define STOPPED 1
                sim_state = (sim_state & 0xFFFFFF00) | 0x01;

                // put to lowest priority.
                if(SIM_VIRT_CORES != 0 && state_ptr){
                    u32 cpu = p->thread_info.cpu;
                    if(cpu < MAX_CORES && cpu >= 0){
                        state_ptr->reversed_priority = ((u64)(cpu_sched_round_cnt[cpu]) << 32) | MAX_PRIORITY;
                        cpu_sched_stats_dec_total(cpu, RESCHEDULE_PENALTY);
                    }
                }
            }

            LOCK;
            if(enq_q == Q1){
                atomic_dec_f1_dec_f2(TOTAL_CNT_SHIFT, Q1_CNT_SHIFT);
            }else if(enq_q == Q2){
                atomic_dec_f1_dec_f2(TOTAL_CNT_SHIFT, Q2_CNT_SHIFT);
            }
            UNLOCK;
        }else{
            if(p->scx.slice == 0){
                if(enq_q == Q1){
                    atomic_dec_cnt(Q1_CNT_SHIFT);
                }else if(enq_q == Q2){
                    atomic_dec_cnt(Q2_CNT_SHIFT);
                }
                sim_state = (sim_state & 0xFFFFFF00) | 0x03;
            }
        }
        if(state_ptr){
            state_ptr->sim_state = sim_state;
        }   
        
        DEBUG_PRINT("<Stopping>(%lu)Target PID %d is stopping state (prev state %d, state_now %d, slice %d): R %d state %d U %d I %d S %d T %d D %d\n", bpf_ktime_get_ns()/1000, p->pid, prev_state, sim_state & 0x00FF, p->scx.slice, runnable, p->__state, p->__state & TASK_UNINTERRUPTIBLE, p->__state & TASK_INTERRUPTIBLE, p->__state & TASK_STOPPED, p->__state & TASK_TRACED, p->__state & TASK_DEAD);
        return;
    }

   if(is_target_pid == -1){
        if(!runnable){
            if(p->__state & TASK_TRACED){
                struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
                u64 sim_state = 0;
                if(state_ptr){
                    sim_state = state_ptr->sim_state;
                }
                #define TRACED 2
                sim_state = (sim_state & 0xFFFFFF00) | 0x02;
                if(state_ptr){
                    state_ptr->sim_state = sim_state;
                }  
            }
        }
    }
}

void BPF_STRUCT_OPS(simple_runnable, struct task_struct *p, u64 enq_flags){
#ifdef DEBUG_RUNNABLE
    int is_target_pid = target_pid_from_userland(p);
    if(is_target_pid == 1){
        DEBUG_PRINT("(%lu) Target PID %d is runnable\n", bpf_ktime_get_ns()/1000, p->pid);
    }
#endif
}

void BPF_STRUCT_OPS(simple_enable, struct task_struct *p)
{
    bool is_target_pid = target_pid_from_userland_init(p);
    u32 index = (u32)p->pid;
    if(is_target_pid == 1){
        u32 cpu = calc_cpu_balanced(p, -1); 
        DEBUG_PRINT("%d ENABLE @init_cpu %d\n", p->pid, cpu);
        #define INIT 255
        // u64 sim_state = INIT;
        struct pstate state;
        state.sim_state = INIT;
        state.pin_cpu = cpu;
        state.reversed_priority = MAX_PRIORITY;
        state.ctrl_msg = 0;
        state.jailbreak = 0;
        bpf_map_update_elem(&sim_proc_state, &index, &state, BPF_ANY);
    }
}

void BPF_STRUCT_OPS(simple_disable, struct task_struct *p)
{
    bool is_target_pid = target_pid_from_userland(p);
    u32 index = (u32)p->pid;
    if(is_target_pid == 1){
        DEBUG_PRINT("%d DISABLE\n", p->pid);
        struct pstate* state_ptr = bpf_map_lookup_elem(&sim_proc_state, &index);
        u64 sim_state = 0;
        u32 pin_cpu = 0;
        if(state_ptr){
            sim_state = state_ptr->sim_state;
            pin_cpu = state_ptr->pin_cpu;
        }
        if(pin_cpu >= 0 && pin_cpu < NR_CORES){
            ATOMIC_MINUS_ONE(pin_core_cnt[pin_cpu]);
        }

        u32 enq_q = (sim_state & 0xFF00)>>8;
        if((sim_state & 0xFF) == 0){
            bpf_printk("WARN: disabled (%lu)(%d, prev state %d) Target PID %d is running slice %d\n", bpf_ktime_get_ns()/1000, enq_q, sim_state & 0xFF, p->pid, p->scx.slice);
        }
        bpf_map_delete_elem(&sim_proc_state, &index);
    }
}

s32 BPF_STRUCT_OPS_SLEEPABLE(simple_init)
{
    on_off = DEFAULT_ON_OFF;
    if(on_off == 0){
        u32 _index = 0;
        u64 _state = 0;
        bpf_map_update_elem(&bpf_sched_ctrl, &_index, &_state, BPF_ANY);
        TIME_QUANTUM_TO_USE = NORMAL_QUANTUM;
        DYN_TIME_QUANTUM_TO_USE = NORMAL_QUANTUM;
    }else{
        u32 _index = 0;
        u64 _state = 1;
        bpf_map_update_elem(&bpf_sched_ctrl, &_index, &_state, BPF_ANY);
        TIME_QUANTUM_TO_USE = TIME_QUANTUM;
        DYN_TIME_QUANTUM_TO_USE = TIME_QUANTUM;
    }

    reset_vts();

    s32 ret = 0;
    ret = ret | scx_bpf_create_dsq(OTHER_DSQ_CONFLICT, -1);
    ret = ret | scx_bpf_create_dsq(OTHER_DSQ, -1);
    ret = ret | scx_bpf_create_dsq(SHARED_DSQ+Q1-1, -1);
    ret = ret | scx_bpf_create_dsq(SHARED_DSQ+Q2-1, -1);

    for(int i = 0; i < NR_CORES; i++){
        // two queues each core
        // q1 number is i*2+START_Q + Q1 - 1
        // q2 number is i*2+START_Q + Q2 - 1
        ret = ret | scx_bpf_create_dsq(i*2+START_Q+Q1-1, -1);
        ret = ret | scx_bpf_create_dsq(i*2+START_Q+Q2-1, -1);
    }

    consume_q = Q1;

    for(int i = 0; i < NR_CORES; i++){
        u32 key = i;
        struct monitor_timer *ptr;
        ptr = bpf_map_lookup_elem(&monitor_timer_arr, &key);
        if (!ptr)
            return -ESRCH;

        bpf_timer_init(&(ptr->timer), &monitor_timer_arr, CLOCK_MONOTONIC);
        bpf_timer_set_callback(&(ptr->timer), monitor_timerfn);
    }

    // for custom scheduling

    if(SIM_VIRT_CORES != 0){
        cpu_sched_stats_init();
    }
    return ret;
}

void BPF_STRUCT_OPS(simple_exit, struct scx_exit_info *ei)
{
    bpf_printk("(not used) stats at exit: average over diff %lu, average less diff %lu, rounds %lu\n", total_over_diff/round64, total_less_diff/round64, round64);
    UEI_RECORD(uei, ei);
}

SCX_OPS_DEFINE(simple_ops,
           .select_cpu      = (void *)simple_select_cpu,
           .enqueue         = (void *)simple_enqueue,
           .dispatch        = (void *)simple_dispatch,
           .running         = (void *)simple_running,
           .stopping        = (void *)simple_stopping,
           .runnable        = (void *)simple_runnable,
           .enable          = (void *)simple_enable,
           .disable         = (void *)simple_disable,
           .init            = (void *)simple_init,
           .exit            = (void *)simple_exit,
           .flags     = SCX_OPS_ENQ_LAST,
           .name            = "simple");

