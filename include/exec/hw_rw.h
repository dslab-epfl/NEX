#pragma once
#include <exec/exec.h>
#include <sched.h>
#include <sims/sim.h>
extern void *mmio_base;
#define MMIO_SIZE 4096*10*4

void* eager_sync_accelerator_manager(void*);

extern int eager_sync_stop;

void handle_hw_fault(pid_t child, int type);
void handle_hw_free_resources(pid_t child);
void set_mmio_to_user();

ssize_t read_process_memory(int fd, uintptr_t address, void *buffer, size_t size);
ssize_t write_process_memory(int fd, uintptr_t address, void *buffer, size_t size);

void bpf_sched_update_state(uint64_t value);