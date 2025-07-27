#pragma once
#include <stdint.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

typedef struct __attribute__((packed)) SchedRegs {
  uint64_t ctrl;
  uint64_t lock;
} SchedRegs;

static void sched_ctrl_lock(SchedRegs* sched_reg) {
    while (__sync_val_compare_and_swap(&sched_reg->lock, 0, 1) != 0) {
        // Busy wait
        sched_yield();
    }
}

static void sched_ctrl_unlock(SchedRegs* sched_reg) {
    __sync_lock_release(&sched_reg->lock);
}

static int sched_my_thread_id(void) {
  return syscall(SYS_gettid);
}