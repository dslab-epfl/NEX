#include <exec/exec.h>
#include <exec/ptrace.h>
#include <exec/syscall.h>
#include <stdint.h>

extern uint64_t read_vts();

void handle_sendto(int waited_child);

void handle_netsyscall(int nr_call, int waited_child){
    if(nr_call == __NR_sendto){
        handle_sendto(waited_child);
    }
    return;
}

void handle_sendto(int waited_child) {

    struct timespec ts1;
    clock_gettime(CLOCK_REALTIME, &ts1);

    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, waited_child, NULL, &regs);
    // printf("Intercepted Syscall: %lld\n", regs.orig_rax);
    regs.orig_rax = -1;
    regs.rax = 1024;
    ptrace(PTRACE_SETREGS, waited_child, NULL, &regs);

    unsigned long sockfd = regs.rdi;
    unsigned long buf = regs.rsi;
    unsigned long len = regs.rdx;
    unsigned long flags = regs.r10;
    unsigned long dest_addr = regs.r8;
    unsigned long addrlen = regs.r9;

    printf("sockfd: %lu, buf: %lx, len: %lu, flags: %lu, dest_addr: %lx, addrlen: %lu\n",
        sockfd, buf, len, flags, dest_addr, addrlen);
    
    char message[1024] = {0};
    for (unsigned long i = 0; i < len && i < 1024; i += sizeof(long)) {
        long word = ptrace(PTRACE_PEEKDATA, waited_child, buf + i, NULL);
        memcpy(message + i, &word, sizeof(long));
    }
    printf("Message: %s\n", message);

    struct sockaddr_in dest;
    for (unsigned long i = 0; i < sizeof(dest); i += sizeof(long)) {
        long word = ptrace(PTRACE_PEEKDATA, waited_child, dest_addr + i, NULL);
        memcpy((char *)&dest + i, &word, sizeof(long));
    }
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(dest.sin_addr), ip, INET_ADDRSTRLEN);

    printf("Destination IP: %s\n", ip);
    printf("Destination Port: %d\n", ntohs(dest.sin_port));

    char execCommand[2048];
    snprintf(execCommand, sizeof(execCommand), "/home/ubuntu/tarballs/ns-allinone-3.42/ns-3.42/ns3 run first \
    -- --message='%s' --length=%ld --destIp='%s' --destPort=%d --dataRate='%s' --delay='%s' \
    2>&1", message, len, ip, ntohs(dest.sin_port), "1Kbps", "10ms");
    const char *ns3Executable = execCommand;
    FILE *pipe = popen(ns3Executable, "r");
    if (pipe == NULL) {
        perror("popen");
        return;
    }

    char result[20][128];

    int i=0;
    while (fgets(result[i], sizeof(result[i]), pipe) != NULL) {
        i++;
    }
    long end = strtol(result[i-1],NULL,10), start = strtol(result[i-4],NULL,10);;
    long simulated_duration = end - start;

    struct timespec ts2;
    clock_gettime(CLOCK_REALTIME, &ts2);
    long diff = (long)(ts2.tv_sec-ts1.tv_sec)*(1e9);
    printf("Real time difference: %ld nsecs, Simulated time difference: %ld nsecs\n", diff+(long)(ts2.tv_nsec - ts1.tv_nsec), simulated_duration);

    if (pclose(pipe) == -1) {
        perror("pclose");
        return;
    }
    uint64_t virtual_time = read_vts();
    // How to access TIME_QUANTUM here, instead of hardcoding its value?
    uint64_t expiration_time = virtual_time - 2000 + simulated_duration; //previous vts + simulated_duration
    
    struct event new_evnt, resume_sched_evnt;
    new_evnt.pid = waited_child;
    new_evnt.vts = expiration_time;
    new_evnt.type = 1;
    
    resume_sched_evnt.pid = -1;
    resume_sched_evnt.vts = virtual_time;
    resume_sched_evnt.type = 0;

    // struct event vts_q_data[1024];
    // int qlen = 0;
    // int event_q_fd = bpf_obj_get("/sys/fs/bpf/event_q");
    // vts_q_data[qlen++] = new_evnt;
    // vts_q_data[qlen++] = resume_sched_evnt;

    struct event vts_q_data[2];
    int qlen = 0;
    vts_q_data[qlen++] = new_evnt;
    vts_q_data[qlen++] = resume_sched_evnt;

    /* sorting events in the queue */ 
    insert_event(vts_q_data, 2);

    // struct event event_q_data;
    // while(bpf_map_lookup_and_delete_elem(event_q_fd, NULL, &event_q_data)==0) {
    //     vts_q_data[qlen++] = event_q_data;
    // }
    // qsort(vts_q_data, qlen, sizeof(struct event), compare_events);
    // for(int i=0;i<qlen;i++) {
    //     bpf_map_update_elem(event_q_fd, NULL, &vts_q_data[i], BPF_ANY);
    // }
    
    /* ends sorting */
    printf("updated ns_timer map with (PID: %d, vtsmp: %lu) at vts: %lu, and ns3_is_running_map with 100\n", waited_child, expiration_time, read_vts());
}