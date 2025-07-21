#include <exec/exec.h>
#include <exec/bpf.h>
#include <exec/ptrace.h>
#include <exec/syscall.h>
#include <stdint.h>
#include <exec/thread_state.h>
#include <errno.h>

int get_syscall_entry_real_time_map_fd() {
    static int fd = -1;  // Static variable, initialized only once

    if (fd == -1) {       // if the file descriptor is not yet initialized
        fd = get_bpf_map("syscall_entry_real_time_map");
        if (fd == -1) {
            perror("bpf map");
        }
    }
    return fd; 
}


void handle_fsync_entry(int waited_child) {
    syscall_entry_time_map_fd = get_syscall_entry_real_time_map_fd(); 
    uint64_t curr_vts = read_vts();

    modify_thread_halt_time(waited_child, curr_vts);
    
    struct timespec real_ts;
    clock_gettime(CLOCK_MONOTONIC, &real_ts);  
    uint64_t real_ts_in_ns = real_ts.tv_sec * (1e9) + real_ts.tv_nsec;

    ptrace(PTRACE_SYSCALL, waited_child, 0, 0);
    
    struct timespec ptrace_ts;
    clock_gettime(CLOCK_MONOTONIC, &ptrace_ts);
    uint64_t ptrace_ts_in_ns = ptrace_ts.tv_sec * (1e9) + ptrace_ts.tv_nsec;
    
    real_ts_in_ns += (ptrace_ts_in_ns - real_ts_in_ns) / 2 ; // a correction factor, add half of the ptrace call time

    // printf("b4_time: %ld, af_time: %ld, ptrace time: %ld\n", b4_ts_in_ns, af_ts_in_ns,  (af_ts_in_ns - b4_ts_in_ns));
    put_bpf_map(syscall_entry_time_map_fd, &waited_child, &real_ts_in_ns, BPF_MAP_UPDATE);
}


bool check_and_handle_fsync_exit(int waited_child) {
    struct timespec real_ts2;
    clock_gettime(CLOCK_MONOTONIC, &real_ts2);
    uint64_t real_ts1, real_ts2_in_ns = real_ts2.tv_sec * (1e9) + real_ts2.tv_nsec;

    syscall_entry_time_map_fd = get_syscall_entry_real_time_map_fd();
    int ret = put_bpf_map(syscall_entry_time_map_fd, &waited_child, &real_ts1, BPF_MAP_LOOKUP_DELETE);
    if(ret == 0) {
        uint64_t fsync_time = real_ts2_in_ns - real_ts1 - 500000; // correction factor 500us, to account for the time between actual fsync exit and ts calculation in this function
        uint64_t syscall_start_vts = get_thread_halt_time(waited_child);
        uint64_t thread_cont_vts = syscall_start_vts + fsync_time;
        modify_thread_halt_time(waited_child, thread_cont_vts);
        ptrace(PTRACE_CONT, waited_child, 0, 0);
        return true;
    }
    return false;    
}
