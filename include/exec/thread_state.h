#pragma once

#include <stdint.h>
#include <stdio.h>
#include <exec/bpf.h>

extern int thread_state_map_fd;

void modify_thread_halt_time(int thread_pid, uint64_t timestamp);

uint64_t get_thread_halt_time(int thread_pid);