#include <stdatomic.h>
#include <stdbool.h>
#include <sched.h>
#include <exec/hw.h>

void nex_lock_init(nex_lock_t *lock) {
    atomic_flag_clear(&lock->flag);
}

void nex_lock_acquire(nex_lock_t *lock) {
    while (atomic_flag_test_and_set_explicit(&lock->flag, memory_order_acquire)) {
        sched_yield();
    }
}

void nex_lock_release(nex_lock_t *lock) {
    atomic_flag_clear_explicit(&lock->flag, memory_order_release);
}