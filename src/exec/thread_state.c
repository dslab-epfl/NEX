#include <exec/thread_state.h>

int get_thread_state_map_fd() {
    static int fd = -1;  // Static variable, initialized only once

    if (fd == -1) {       // if the file descriptor is not yet initialized
        fd = get_bpf_map("thread_state_map");
        if (fd == -1) {
            fprintf(stderr, "Error getting BPF vts map\n");
            return -1;
        }
    }
    return fd; 
}

void modify_thread_halt_time(int thread_pid, uint64_t timestamp) {
    struct per_thread_state thread_state;
    thread_state.halt_until = timestamp;
    thread_state_map_fd = get_thread_state_map_fd();
    put_bpf_map(thread_state_map_fd, &thread_pid, &thread_state, BPF_MAP_UPDATE);
    return;
}

uint64_t get_thread_halt_time(int thread_pid) {
    struct per_thread_state thread_state;
    thread_state_map_fd = get_thread_state_map_fd();
    if(put_bpf_map(thread_state_map_fd, &thread_pid, &thread_state, BPF_MAP_LOOKUP) == 0) {
        return thread_state.halt_until;
    }
    return 0; 
}