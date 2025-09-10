#include <exec/exec.h>
#include <exec/bpf.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <execinfo.h>

// #define EXEC_DEBUG 
// turn off debug print, otherwise it can deadlock
#ifdef EXEC_DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

int nex_pid = 0;
int sim_end = 0;
uint64_t sys_up_time;

int from_nex_runtime_event_q_fd;
int to_nex_runtime_event_q_fd;
int sim_proc_state_fd;
int trace_event_q_fd;
int bpf_sched_ctrl_fd;
int syscall_entry_real_time_map_fd;
int thread_state_map_fd;
int event_q_fd;
int vts_fd;

int eager_sync_stop;
void *mmio_base;

static
int create_smem(const char *shm_name, int size, int init) {
    int shm_fd;
   
    printf("init %s\n", shm_name);
    printf("Creating shared memory %s\n", shm_name);
    if (init == 0) {
        printf("Open/not create %s\n", shm_name);
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        DEBUG("Open/not create %s shm_fd %d\n", shm_name, shm_fd);
        if (shm_fd == -1) {
            perror("shm_open");
            return -1;
        }
    }else{
        printf("Open create %s\n", shm_name);
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

static
void* init_mmio_region(const char *shm_name, int size, int init, int* fd){
    int shm_fd = create_smem(shm_name, size, init);
    printf("shm_fd %d\n", shm_fd);
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

#define SCHED_EXT 7

static const int MAX_FRAMES = 100;
static void
crash_handler(int sig, siginfo_t *si, void *unused)
{
    printf("Caught signal %d (%s) at address %p",
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

volatile sig_atomic_t stop = 0;

void handle_sigint(int sig) {
    stop = 1;
}

int main(int argc, char *argv[]) {
    
    int on_off = -1;
    
    if(argc == 2){
        on_off = atoi(argv[1]);
    }

    if (geteuid() != 0) {
        printf("\033[1;33m⚠️  Warning: This program should be run with sudo/root privileges.\033[0m\n");
    }
    // autotuning phrase
    nex_pid = getpid();

    #if CONFIG_ENABLE_BPF
    if (CONFIG_EXTRA_COST_TIME == 0){
        printf("Run \"make autoconfig\" first to configure CONFIG_EXTRA_COST_TIME\n");
        exit(0);
    }
    #endif

    install_crash_handler();

    #if CONFIG_ENABLE_BPF
    attach_bpf(-1, -1, on_off);

    // struct timeval ts;
    // gettimeofday(&ts, NULL);
    // sys_up_time = ts.tv_sec * 1000000000LL + ts.tv_usec * 1000LL;
    // printf("System up time (ns): %lu\n", sys_up_time);
    // set_vts(sys_up_time);
    eager_sync_stop = 0;
    #if CONFIG_EAGER_SYNC
    from_nex_runtime_event_q_fd = get_bpf_map("from_nex_runtime_event_q");
    to_nex_runtime_event_q_fd = get_bpf_map("to_nex_runtime_event_q");
    #endif
    sim_proc_state_fd = get_bpf_map("sim_proc_state");
    #endif

    // Wait until interrupted (SIGINT/SIGTERM), then clean up
    struct sigaction sa_int = {0};
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);
    sigaction(SIGTERM, &sa_int, NULL);

    printf("\033[1;32m✔ NEX server running (default EBS %d). Press Ctrl+C to exit. \033[0m\n", on_off);
    while (!stop) pause();

    #if CONFIG_ENABLE_BPF
    destroy_bpf();
    #endif

    printf("\033[1;32m✔ NEX server stopped.\033[0m\n");

    return 0;
}

#if !CONFIG_ENABLE_BPF
uint64_t read_vts(){
    //read actual time 
    return 0;
    // return get_real_ts();
}

uint64_t set_vts(uint64_t value){
    //read actual time 
    return 0;
    // return get_real_ts();
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