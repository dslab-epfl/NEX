#pragma once

#include <exec/bpf.h>
struct event {
    unsigned long long vts;
    unsigned int pid;
    unsigned int type;
};

int compare_events(const void *evnt_a, const void *evnt_b); 

int insert_event(struct event *evnt, int nr);