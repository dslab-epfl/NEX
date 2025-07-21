#pragma once 
#include <sys/prctl.h>
#include <syscall.h> 
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <stddef.h>
#include <sys/reg.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <exec/evnt.h>
#include <exec/bpf.h>

#define SYSCALL_ARG1_REG(regs) regs.rdi
#define SYSCALL_ARG2_REG(regs) regs.rsi
#define SYSCALL_ARG3_REG(regs) regs.rdx
#define SYSCALL_ARG4_REG(regs) regs.r10
#define SYSCALL_ARG5_REG(regs) regs.r8

extern int syscall_entry_time_map_fd;

void handle_netsyscall(int nr_call, int waited_child);

void handle_fsync_entry(int waited_child);

bool check_and_handle_fsync_exit(int waited_child);

void handle_clock_gettime(int child);
void handle_gettimeofday(int child);
void handle_clock_nanosleep(int child);

