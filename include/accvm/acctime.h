#ifndef ACCVM_ACCTIME_H
#define ACCVM_ACCTIME_H
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <accvm/context.h>
#include <sys/types.h>

#define COMPENSATE_TIME 21

extern uint64_t sys_up_time;

extern int vts_fd;
extern int bpf_sched_ctrl_fd;
extern int sim_proc_state_fd;

extern int (*orig_nanosleep)(const struct timespec *req, struct timespec *rem);
extern int (*orig_clock_nanosleep)(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);
extern int (*orig_clock_gettime)(clockid_t, struct timespec *);
extern int (*orig_gettimeofday)(struct timeval *, void *);
// Wrapper around gettimeofday for concise code, in microseconds
uint64_t get_real_ts();
// Microseconds to timeval
void usToTimeval(uint64_t microseconds, struct timeval *time);

void vm_adjust_vm_time(t_context *ctx, uint64_t *entry_ts);

void vm_entry(t_context *ctx, uint64_t *entry_ts);

void vm_exit(t_context *ctx, uint64_t *entry_ts);

// Preload overload of gettimeofday
int gettimeofday(struct timeval *tv, void *tz);

void ns_to_timeval(uint64_t ns, struct timeval *tv); 

uint64_t read_vts();

int penalize_thread(int pid, uint64_t till);

uint64_t slowdown();
#endif
