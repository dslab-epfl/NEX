#pragma once
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>   // For constants and structures needed for ptrace
#include <linux/perf_event.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sched.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <bits/stdint-uintn.h>
#include <config/config.h>
#include <stdatomic.h>



void hw_init();
void hw_deinit();
void safe_printf(const char *fmt, ...);

typedef struct {
    atomic_flag flag;
} nex_lock_t;

void nex_lock_init(nex_lock_t *lock);

void nex_lock_acquire(nex_lock_t *lock);

void nex_lock_release(nex_lock_t *lock);

#if CONFIG_DEBUG_HARMLESS
    #define DEBUG(...) safe_printf(__VA_ARGS__)
#else
    #define DEBUG(...)
#endif

#if CONFIG_DEBUG_HARM
    #define DEBUG_H(...) safe_printf(__VA_ARGS__)
#else
    #define DEBUG_H(...)
#endif

#if CONFIG_DEBUG_MMIO
    #define DEBUG_MMIO(...) safe_printf(__VA_ARGS__)
#else
    #define DEBUG_MMIO(...)
#endif
