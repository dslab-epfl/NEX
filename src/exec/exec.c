#include <exec/exec.h>
#include <exec/ptrace.h>
#include <exec/hw_rw.h>
#include <exec/hw.h>
#include <exec/syscall.h>
#include <exec/bpf.h>
#include <signal.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <execinfo.h>
#include <string.h>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/user.h>
#include <stdlib.h>

// #define EXEC_DEBUG 
// turn off debug print, otherwise it can deadlock
#ifdef EXEC_DEBUG
#define DEBUG_PRINT(...) safe_printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

// static int ctx_switch_off = 0;
static char mmio_shm_name[50];
static int fds[5];

int nex_pid = 0;
int sim_end = 0;
uint64_t sys_up_time;
int eager_sync_stop;
void *mmio_base;

int from_nex_runtime_event_q_fd;
int to_nex_runtime_event_q_fd;
int sim_proc_state_fd;
int trace_event_q_fd;
int bpf_sched_ctrl_fd;
int syscall_entry_real_time_map_fd;
int thread_state_map_fd;
int event_q_fd;
int vts_fd;

#if CONFIG_ENABLE_BPF
int handle_if_rdtsc(int waited_child);
#endif

extern void bpf_sched_update_state_per_pid(uint32_t ctrl_pid, uint32_t ctrl_msg);

void *poll_trace_eventq(void *arg) {
    // while (1) {
    //     struct trace_evnt evnt;
    //     if(put_bpf_map(trace_event_q_fd, NULL, &evnt, BPF_MAP_LOOKUP_DELETE) == 0){
    //         // safe_printf("calling ptrace bpf peek for pid: , vts: %lu via process: %d\n", read_vts(), getppid());
    //         char buf[100];
    //         int len = snprintf(buf, sizeof(buf), "%d,%lu,%lu\n", evnt.type, evnt.ts, evnt.data);
    //         write(log_file_fd, buf, len);
    //     }else{
    //         if(sim_end){
    //             return NULL;
    //         }
    //     }
    //     usleep(10);
    // }
    // return NULL;
    return NULL;
}


int create_smem(const char *shm_name, int size, int init) {
    int shm_fd;
    safe_printf("init %s\n", shm_name);
    if (init == 0) {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        DEBUG("Open/not create %s shm_fd %d\n", shm_name, shm_fd);
        if (shm_fd == -1) {
            perror("shm_open");
            return -1;
        }
    }else{
        shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open");
            return -1;
        }
        if (ftruncate(shm_fd, size) == -1) {
            perror("ftruncate");
            return -1;
        }
    }
    return shm_fd;
}

void* init_mmio_region(const char *shm_name, int size, int init, int* fd){
    int shm_fd = create_smem(shm_name, size, init);
    *fd = shm_fd;
    // no protection
    void* mmio_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mmio_base == MAP_FAILED) {
        perror("Failed to map MMIO region");
        exit(EXIT_FAILURE);
    }
    if (init != 0) {
        memset(mmio_base, 0, size);
    }

    return mmio_base;
}

void init(int host_id){
  sprintf(mmio_shm_name, "nex_mmio_regions");
  mmio_base = init_mmio_region(mmio_shm_name, MMIO_SIZE, 1, &fds[0]);
  hw_init();
}

int inside_hw_sim = 0;
int lpn_state = 0;

#define SCHED_EXT 7

uint64_t get_time(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static const int MAX_FRAMES = 100;
static void
crash_handler(int sig, siginfo_t *si, void *unused)
{
    safe_printf("Caught signal %d (%s) at address %p",
        sig, strsignal(sig), si->si_addr);
    void *frames[MAX_FRAMES];
    int  frame_count;
    int  log_fd;

    /* Open fresh crash log */
    // change file name to be pid crash_trace.log
    char log_filename[256];
    snprintf(log_filename, sizeof(log_filename), "crash_trace_%d.log", getpid());
    log_fd = open(log_filename,
                  O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (log_fd < 0) log_fd = STDERR_FILENO;

    /* Signal info */
    dprintf(log_fd,
            "ERROR: signal %d (%s) at address %p\n",
            sig, strsignal(sig), si->si_addr);

    /* Backtrace */
    frame_count = backtrace(frames, MAX_FRAMES);
    backtrace_symbols_fd(frames, frame_count, log_fd);

    unsetenv("LD_PRELOAD");
    /* For each frame, resolve via addr2line (full path) and write only that */
   for (int i = 0; i < frame_count; ++i) {
        Dl_info info;
        dladdr(frames[i], &info);
        const char *obj = info.dli_fname ?: "/proc/self/exe";
        uintptr_t base = (uintptr_t)info.dli_fbase;
        uintptr_t addr = (uintptr_t)frames[i];
        uintptr_t offset = addr - base;

        char cmd[512];
        snprintf(cmd, sizeof(cmd),
            "/usr/bin/addr2line -f -p -e %s 0x%" PRIxPTR,
            obj, offset);

        FILE *fp = popen(cmd, "r");
        if (fp) {
            char buf[512];
            while (fgets(buf, sizeof(buf), fp)) {
                dprintf(log_fd, "    src: %s", buf);
            }
            pclose(fp);
        }
    }
    if (log_fd != STDERR_FILENO) close(log_fd);
    _exit(1);
}

static void
install_crash_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags     = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
}

int main(int argc, char *argv[]) {
    
    // on bpf errors without sudo; try this, sudo chmod 0711 /sys/fs/bpf
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    nex_pid = getpid();

    #if CONFIG_ENABLE_BPF
    map_bpf();
    #endif

    // child inherent the scheduling policy
    
    printf("NEX exec running %d \n", nex_pid);

    struct sched_param sp = { .sched_priority = 0 };
    sched_setscheduler(nex_pid, SCHED_EXT, &sp);

    uint64_t start_ts, end_ts;

    init(0);

    install_crash_handler();

    pid_t busy_loop_pid = fork();
    if (busy_loop_pid == 0) {
        // Child process: the busy loop
        // while(1){
        //     volatile int x = 0;
        // }
        exit(0);
    }


    pid_t dp = fork();
    pid_t tracee=-1;
    pthread_t eager_sync_thread_id;
    if(dp==0) {
        raise(SIGSTOP);
            // Child process: the tracee
        safe_printf("Tracee pid: %d\n", getpid());

        // Trap RDTSC by delivering SIGSEGV when executed (x86/x86_64 only)
        #if CONFIG_ENABLE_BPF && defined(__x86_64__)
        if (prctl(PR_SET_TSC, PR_TSC_SIGSEGV) == -1) {
            perror("prctl(PR_SET_TSC, PR_TSC_SIGSEGV)");
        }
        #endif

        #define INTERCEPT_SYSCALL(name) \
            BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##name, 0, 1), \
            BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_TRACE), \
            BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

        // struct sock_filter filter[] = {
        //     BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct seccomp_data, nr)),
        //     // INTERCEPT_SYSCALL(sendto),
        //     // INTERCEPT_SYSCALL(fsync),
        //     INTERCEPT_SYSCALL(clock_gettime),
        //     // INTERCEPT_SYSCALL(gettimeofday),
        //     // INTERCEPT_SYSCALL(nanosleep),
        // };
        // struct sock_fprog prog = {
        //     .filter = filter,
        //     .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
        // };

        // /* To avoid the need for CAP_SYS_ADMIN */
        // if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
        //     perror("prctl(PR_SET_NO_NEW_PRIVS)");
        //     return 1;
        // }

        // if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == -1) {
        //     perror("when setting seccomp filter");
        //     return 1;
        // }

        // struct sched_param param;
        // if (sched_setscheduler(getpid(), SCHED_EXT, &param) == -1) {
        //     perror("sched_setscheduler");
        //     return EXIT_FAILURE;
        // }
        
        DEBUG_PRINT("Child, PID: %d\n", getpid());
        int env_count = 0;
        while (environ[env_count] != NULL) {
            env_count++;
        }
        char **new_env = malloc((env_count + 4) * sizeof(char *));
        if (new_env == NULL) {
            perror("malloc failed");
            return 1;
        }
        for (int i = 0; i < env_count; i++) {
            new_env[i] = environ[i];
        }

        new_env[env_count] = malloc(200);
        memset(new_env[env_count], 0, 200);
        DEBUG_PRINT("CONFIG_PROJECT_PATH %s\n", CONFIG_PROJECT_PATH);
        
        char ld_preload_str[200] = "LD_PRELOAD=";
        //append to the ld_preload string if cuda enabled 

        #if CONFIG_ENABLE_BPF
            snprintf(ld_preload_str + strlen(ld_preload_str), 
                    sizeof(ld_preload_str) - strlen(ld_preload_str), 
                    "%s/%s", CONFIG_PROJECT_PATH, 
                    "src/accvm.so:");
        #endif

        #if CONFIG_GPU
            snprintf(ld_preload_str + strlen(ld_preload_str), 
                    sizeof(ld_preload_str) - strlen(ld_preload_str), 
                    "%s/%s", CONFIG_PROJECT_PATH, 
                    "src/sims/gpu/nex_cuda.so");
        #endif

        #if CONFIG_ENABLE_BPF || CONFIG_GPU
            strcpy(new_env[env_count], ld_preload_str);
            safe_printf("LD_PRELOAD: %s\n", new_env[env_count]);
        #endif

        #if CONFIG_GPU
            new_env[env_count+1] = malloc(200);
            sprintf(new_env[env_count+1], "REPLACE_LIB=%s/%s", CONFIG_PROJECT_PATH, "src/sims/gpu/nex_cuda.so");
        #endif

        set_mmio_to_user();
        new_env[env_count+3] = NULL;

        printf("Set sched_class to SCHED_EXT, now it will exec %s\n", argv[1]);

        execvpe(argv[1], argv + 1, new_env);
        perror("execvpe");
        return EXIT_FAILURE;
    } else {

        int status;
        
        // Parent process: the tracer
        int waited_pid = waitpid(-1, &status, WUNTRACED);
        
        // Stopped
        // goto ABS_END;
        
        if(waited_pid == dp){
            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
                safe_printf("Child %d has stopped and is ready to be traced.\n", waited_pid);
                ptrace(PTRACE_SEIZE, waited_pid, 0, 0);
                // ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT);
                // ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT);
                ptrace(PTRACE_SETOPTIONS, waited_pid, NULL, PTRACE_O_TRACESECCOMP | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC);
                ptrace(PTRACE_CONT, waited_pid, 0, 0);
                safe_printf("Child cont. %d\n", waited_pid);
            }
        }

        tracee = waited_pid;

        // stop for exec, execvpe
        int ret = waitpid(tracee, &status, 0);
        assert(ret != -1);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            safe_printf("Child %d has stopped at first exec.\n", waited_pid);
            start_ts = get_time();
            ptrace(PTRACE_CONT, tracee, 0, 0);
            safe_printf("Child %d cont. \n", waited_pid);
        }else{
            printf("Tracee error: %d\n", WSTOPSIG(status));
            assert(0);
        }

        #ifdef CONFIG_EAGER_SYNC
            if(CONFIG_EAGER_SYNC){
                if (pthread_create(&eager_sync_thread_id, NULL, eager_sync_accelerator_manager, NULL) != 0) {
                    perror("Failed to create thread");
                    return 1;
                }
            }
        #endif

        while (1) {
            // safe_printf("Waiting for child\n");
            safe_printf("Auto resolve deadlock set threshold for 10 tries; (set to 0 turns this off)\n");
            cfg_deadlock_resolve(10);

            int waited_child = 0;
            do{
                waited_child = waitpid(-1, &status, __WALL);
            }while(waited_child == -1 && errno == EINTR);
            
            if (waited_child == -1) {
                perror("waitpid");
                break;
            }
            // safe_printf("waitpid %d with status %d, stopped %d, signal%d\n", waited_child, status, WIFSTOPPED(status), WSTOPSIG(status));
            if (WIFEXITED(status) ) { // Check if child has exited
                handle_hw_free_resources(waited_child);
                DEBUG_PRINT("Tracee exited with status %d\n", WEXITSTATUS(status));
                if(waited_child == tracee){
                    break;
                }
                continue;
            }
            
            if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGCHLD){
                handle_hw_free_resources(waited_child);
                DEBUG_PRINT("Child terminiated\n");
                ptrace(PTRACE_CONT, waited_child, 0, 0);
            }else if (WIFSTOPPED(status) && status >> 8 == (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8))) {
                int nr_syscall = ptrace(PTRACE_PEEKUSER, waited_child , sizeof(long)*ORIG_RAX, 0);
                if(nr_syscall == __NR_fsync){
                    handle_fsync_entry(waited_child);
                }else if(nr_syscall == __NR_clock_gettime){
                    handle_clock_gettime(waited_child);
                }else if(nr_syscall == __NR_nanosleep){
                    handle_clock_nanosleep(waited_child);
                }
                // handle_netsyscall(nr_syscall, waited_child);  
                ptrace(PTRACE_CONT, waited_child, 0, 0); 
            }else if(WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP|0x80)){
                // syscall entry, if tracesysgood is set
                // DEBUG_PRINT("Syscall entry\n");
                // handle_syscall(waited_child);
                ptrace(PTRACE_CONT, waited_child, 0, 0);
            }else if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
                int event = (status >> 16) & 0xffff;
                if (event == PTRACE_EVENT_CLONE || event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK) {
                    safe_printf("clone enter \n");
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
                     safe_printf("clone exit \n");
                }else if (event == PTRACE_EVENT_EXIT){
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
                }else if (event == PTRACE_EVENT_EXEC){
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
                }else if(event == PTRACE_EVENT_STOP){
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
                }else{            
                    ptrace(PTRACE_CONT, waited_child, 0, 0);      
                    // if(!check_and_handle_fsync_exit(waited_child)) { // between syscall entry and exit, there can be PTRACE_EVENTS or child exit, so they have been checked before
                    //     safe_printf("Unkown event %d\n", event);
                    //     assert(0);
                    // }                    
                }
            }else if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGSTOP){
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
            }else if(WIFSTOPPED(status) && WSTOPSIG(status)==SIGSEGV){
                safe_printf("sigsegv enter \n");
                int is_rdtsc = handle_if_rdtsc(waited_child);
                if(!is_rdtsc){
                    handle_hw_fault(waited_child, SIGSEGV);
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
                }
                safe_printf("sigsegv return\n");

            }else if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGILL) {
                safe_printf("SIGILL enter\n");
                handle_hw_fault(waited_child, SIGILL);
                ptrace(PTRACE_CONT, waited_child, 0, 0);
                safe_printf("SIGILL return\n");
            }else if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGUSR1){
                // for interrupts simulation
                // just return
                safe_printf("SIGUSR1 enter\n");
                ptrace(PTRACE_CONT, waited_child, 0, SIGUSR1);
                safe_printf("SIGUSR1 return\n");
            }else if (WIFSTOPPED(status) && WSTOPSIG(status)==SIGFPE){
                safe_printf("SIGFPE enter\n");
                handle_hw_fault(waited_child, SIGFPE);
                safe_printf("SIGFPE return\n");
                ptrace(PTRACE_CONT, waited_child, 0, 0);
            }
            else{
                safe_printf("Unkown reason: child stopped %d\n", WSTOPSIG(status));
                if(SIGINT == WSTOPSIG(status)){
                    ptrace(PTRACE_CONT, waited_child, 0, 0);
                    goto END;
                }
                ptrace(PTRACE_CONT, waited_child, 0, WSTOPSIG(status));
            }
        }
    }

    end_ts = get_time();
    safe_printf("\n===\nExecution time (ms) \n===\n", (end_ts - start_ts)/1000000);
    printf("\n===\nExecution time (ms): %lu \n===\n", (end_ts - start_ts)/1000000);
    //flush the safe_printf
    fflush(stdout);
END:
    ptrace(PTRACE_CONT, dp, 0, 0);

    #ifdef CONFIG_EAGER_SYNC
    if(CONFIG_EAGER_SYNC){
        eager_sync_stop = 1;
        pthread_join(eager_sync_thread_id, NULL);
    }
    #endif

ABS_END:
    sim_end = 1;

    kill(busy_loop_pid, SIGKILL);

    #if CONFIG_ENABLE_BPF
        unmap_bpf();
    #endif

    // hw_deinit();

    DEBUG_PRINT("===== Tracer stopped ===== \n");
    return 0;
}

#if !CONFIG_ENABLE_BPF
uint64_t read_vts(){
	return 0;
}

void cfg_deadlock_resolve(uint64_t threshold){
    return;
}

int get_bpf_map(char* map_name){
	return -1;
}

int put_bpf_map(int map_fd, void* key, void* value, int ops){
	return -1;
}

uint64_t read_err_bound(){
    return 0;
}

int attach_bpf(int pid, int extra_cost, int on_off){
    return 0;
}

int destroy_bpf(){
    return 0;
}

#endif

static uint64_t find_freq_khz(){
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return 0;
    }

    char line[256];
    uint64_t freq_khz = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "cpu MHz : %" SCNu64, &freq_khz) == 1) {
            freq_khz *= 1000; // Convert MHz to kHz
            break;
        }
    }

    fclose(fp);

    if (freq_khz == 0) {
        fprintf(stderr, "Failed to find CPU frequency in /proc/cpuinfo\n");
    }

    return freq_khz;
}

int handle_if_rdtsc(int waited_child){
    
    #if CONFIG_ENABLE_BPF

    safe_printf("handle_if_rdtsc for %d\n", waited_child);
     // Detect if SIGSEGV was caused by RDTSC when PR_SET_TSC=PR_TSC_SIGSEGV
    int is_rdtsc = 0;
    int is_rdtscp = 0;
    unsigned long long rip_val = 0;
    struct user_regs_struct regs;
    int regs_valid = 0;
    #if defined(__x86_64__) 
    {
        if (ptrace(PTRACE_GETREGS, waited_child, 0, &regs) == 0) {
            // safe_printf("PTRACE_GETREGS for %d\n", waited_child);
            regs_valid = 1;
            #ifdef __x86_64__
            rip_val = regs.rip;
            #else
            rip_val = regs.eip;
            #endif
            errno = 0;
            long w = ptrace(PTRACE_PEEKTEXT, waited_child, (void*)rip_val, 0);
            // safe_printf("PTRACE_PEEKTEXT for %d\n", waited_child);
            if (!(w == -1 && errno)) {
                unsigned char b[8];
                memcpy(b, &w, sizeof(w));
                // RDTSC: 0F 31; RDTSCP: 0F 01 F9
                if ((b[0] == 0x0f && b[1] == 0x31) ||
                    (b[0] == 0x0f && b[1] == 0x01)) {
                    if (b[0] == 0x0f && b[1] == 0x31) {
                        is_rdtsc = 1;
                    } else {
                        long w2 = ptrace(PTRACE_PEEKTEXT, waited_child, (void*)(rip_val + 2), 0);
                        unsigned char b2[sizeof(long)];
                        memcpy(b2, &w2, sizeof(w2));
                        if (b2[0] == 0xf9) {
                            is_rdtscp = 1;
                            is_rdtsc = 1;
                        }
                    }
                }
            }
        }
    }
    #endif
    // safe_printf("is_rdtsc %d, is_rdtscp %d, regs_valid %d \n", is_rdtsc, is_rdtscp, regs_valid);
    if (is_rdtsc) {
        static unsigned long long tsc_khz = 0;
        if(tsc_khz == 0)
            tsc_khz = find_freq_khz();
        // safe_printf("Using NEX_TSC_KHZ=%lu\n", tsc_khz);
        uint64_t vts_ns = read_vts();
        __uint128_t prod = (__uint128_t)vts_ns * (__uint128_t)tsc_khz;
        uint64_t tsc_cycles = (uint64_t)(prod / 1000000ULL);
        // safe_printf("vts_ns %lu, tsc_khz %lu, tsc_cycles %lu\n", vts_ns, tsc_khz, tsc_cycles);
        if (regs_valid) {
            regs.rax = (uint32_t)(tsc_cycles & 0xffffffffULL);
            regs.rdx = (uint32_t)((tsc_cycles >> 32) & 0xffffffffULL);
            if (is_rdtscp) {
                // IA32_TSC_AUX value: leave 0 for now
                regs.rcx = 0;
            }
            regs.rip += is_rdtscp ? 3 : 2;
            
            ptrace(PTRACE_SETREGS, waited_child, 0, &regs);
            // safe_printf("Emulated RDTSC/RDTSCP, TSC cycles: %lu\n", tsc_cycles);
            ptrace(PTRACE_CONT, waited_child, 0, 0);
        } else {
            safe_printf("regs not valid !! \n");
            // Fail, propagate the signal
            ptrace(PTRACE_CONT, waited_child, 0, SIGSEGV);
        }
    }
    return is_rdtsc;
#endif
    return 0;
}