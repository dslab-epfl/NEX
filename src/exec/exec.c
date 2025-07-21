#include <exec/exec.h>
#include <exec/ptrace.h>
#include <exec/hw_rw.h>
#include <exec/hw.h>
#include <exec/syscall.h>
#include <exec/bpf.h>
#include <signal.h>
#include <sys/mman.h>

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
int syscall_entry_time_map_fd;
int trace_evnt_fd; 
int log_file_fd;
int from_nex_runtime_event_q_fd;
int to_nex_runtime_event_q_fd;
int sim_proc_state_fd;
int vts_fd;
int bpf_sched_ctrl_fd;
int eager_sync_stop;
void *mmio_base;
int thread_state_map_fd;

void *poll_trace_eventq(void *arg) {
    // while (1) {
    //     struct trace_evnt evnt;
    //     if(put_bpf_map(trace_evnt_fd, NULL, &evnt, BPF_MAP_LOOKUP_DELETE) == 0){
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
    printf("init %s\n", shm_name);
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
  sprintf(mmio_shm_name, "/nex_mmio_regions");
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

int main(int argc, char *argv[]) {
    nex_pid = getpid();
    uint64_t start_ts, end_ts;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init(0);   

    pid_t dp = fork();
    pid_t tracee=-1;
    pthread_t eager_sync_thread_id;
    if(dp==0) {
        raise(SIGSTOP);
        int child = fork();
        if (child == 0) {
            // Child process: the tracee
            safe_printf("Tracee pid: %d\n", getpid());

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
            #if CONFIG_ENABLE_CUDA
                snprintf(ld_preload_str + strlen(ld_preload_str), 
                        sizeof(ld_preload_str) - strlen(ld_preload_str), 
                        "%s/%s", CONFIG_PROJECT_PATH, 
                        "external/cuda_interpose/bin/cricket-client.so:");
            #endif

            #if CONFIG_ENABLE_BPF
                snprintf(ld_preload_str + strlen(ld_preload_str), 
                        sizeof(ld_preload_str) - strlen(ld_preload_str), 
                        "%s/%s", CONFIG_PROJECT_PATH, 
                        "src/accvm.so");
            #endif

            #if CONFIG_ENABLE_BPF || CONFIG_ENABLE_CUDA
                strcpy(new_env[env_count], ld_preload_str);
                safe_printf("LD_PRELOAD: %s\n", new_env[env_count]);
            #endif

            #if CONFIG_ENABLE_CUDA
                new_env[env_count+1] = malloc(200);
                sprintf(new_env[env_count+1], "REMOTE_GPU_ADDRESS=%s", CONFIG_CUDA_GPU_ADDRESS);
                new_env[env_count+2] = malloc(200);
                sprintf(new_env[env_count+2], "LD_LIBRARY_PATH=%s/%s:$LD_LIBRARY_PATH", CONFIG_PROJECT_PATH, "external/cuda_interpose/bin/");
            #endif

            set_mmio_to_user();
            new_env[env_count+3] = NULL;
            safe_printf("Child will exec %s\n", argv[1]);

            execvpe(argv[1], argv + 1, new_env);
            perror("execvpe");
            return EXIT_FAILURE;
        }else{
            raise(SIGSTOP);
            return 0;
        }
    } else {

        int status;
        
        // Parent process: the tracer
        int waited_pid = waitpid(-1, &status, WUNTRACED);
        
        if(waited_pid == dp){
            if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
                ptrace(PTRACE_SEIZE, waited_pid, 0, 0);
                // ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT);
                // ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT);
                ptrace(PTRACE_SETOPTIONS, waited_pid, NULL, PTRACE_O_TRACESECCOMP | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC);
                ptrace(PTRACE_CONT, waited_pid, 0, 0);
            }
        }

        int w_cnt = 0;
        while(w_cnt < 3){
            int waited_pid = waitpid(-1, &status, __WALL);
            w_cnt++;
            if(waited_pid == dp){
                if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP){
                    ptrace(PTRACE_CONT, waited_pid, 0, 0);
                }
            }else{
                tracee = waited_pid;
                ptrace(PTRACE_CONT, tracee, 0, 0);
            }
        }

        assert(tracee != -1);

        // stop for exec
        int ret = waitpid(tracee, &status, 0);
        assert(ret != -1);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            #if CONFIG_ENABLE_BPF
            extern int attach_bpf(int pid);
            attach_bpf(dp);
            eager_sync_stop = 0;
            from_nex_runtime_event_q_fd = get_bpf_map("from_nex_runtime_event_q");
            to_nex_runtime_event_q_fd = get_bpf_map("to_nex_runtime_event_q");
            sim_proc_state_fd = get_bpf_map("sim_proc_state");
            #endif
            log_file_fd = open("log.txt", O_CREAT|O_RDWR|O_TRUNC, 0666);
            start_ts = get_time();
            ptrace(PTRACE_CONT, tracee, 0, 0);
        }else{
            printf("Tracee error: %d\n", WSTOPSIG(status));
            assert(0);
        }

        // pthread_t thread_id;
        // used for eager sync, event tracing, etc. disabled
        // if (pthread_create(&thread_id, NULL, poll_trace_eventq, NULL) != 0) {
        //     perror("Failed to create thread");
        //     return 1;
        // }
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
                handle_hw_fault(waited_child, SIGSEGV);
                ptrace(PTRACE_CONT, waited_child, 0, 0);
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
                ptrace(PTRACE_CONT, waited_child, 0, 0);
            }
        }
    }

    end_ts = get_time();
    extern uint64_t read_err_bound();
    safe_printf("\n===\nExecution time (ms): %lu\nError upper bound (us): %lu\n===\n", (end_ts - start_ts)/1000000, read_err_bound()/1000);
    printf("\n===\nExecution time (ms): %lu\nError upper bound (us): %lu\n===\n", (end_ts - start_ts)/1000000, read_err_bound()/1000);
    //flush the safe_printf
    fflush(stdout);
END:
    ptrace(PTRACE_CONT, dp, 0, 0);
    sim_end = 1;

    #ifdef CONFIG_EAGER_SYNC
    if(CONFIG_EAGER_SYNC){
        eager_sync_stop = 1;
        pthread_join(eager_sync_thread_id, NULL);
    }
    #endif
    hw_deinit();

    #if CONFIG_ENABLE_BPF
    extern int destroy_bpf();
    destroy_bpf();
    #endif

    close(log_file_fd);
    
    pid_t setup_child = fork();
    if(setup_child == 0){
        char command[100];
        sprintf(command, "rm /tmp/*sock*");
        char *args[] = {"/bin/sh", "-c", command, NULL};
        execvp(args[0], args);
        perror("execvp failed");
    }
    
    waitpid(setup_child, NULL, 0);
    DEBUG_PRINT("===== Tracer stopped ===== \n");
    return 0;
}

#if !CONFIG_ENABLE_BPF
uint64_t read_vts(){
	return 0;
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
#endif