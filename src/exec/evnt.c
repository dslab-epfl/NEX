#include <exec/exec.h>
#include <exec/ptrace.h>
#include <exec/evnt.h>
#include <exec/bpf.h>

int compare_events(const void *evnt_a, const void *evnt_b) {
    return ( ((struct event*)evnt_a)->vts - ((struct event*)evnt_b)->vts);
}

int insert_event(struct event *evnt, int nr) {
    struct event vts_q_data[1024];
    int qlen = 0;
    struct event event_q_data;
    for (int i=0; i<nr; i++) {
        vts_q_data[qlen++] = evnt[i];
    }

    int event_q_fd = get_bpf_map("event_q");
    
    while(put_bpf_map(event_q_fd, NULL, &event_q_data, BPF_MAP_LOOKUP_DELETE)==0) {
        vts_q_data[qlen++] = event_q_data;
    }
    qsort(vts_q_data, qlen, sizeof(struct event), compare_events);
    for(int i=0;i<qlen;i++) {
        put_bpf_map(event_q_fd, NULL, &vts_q_data[i], BPF_MAP_UPDATE);
    }
    return 0;
}