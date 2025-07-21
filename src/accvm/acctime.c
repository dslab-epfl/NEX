#include <accvm/context.h>
#include <accvm/thread.h>
#include <accvm/accvm.h>
#include <accvm/acctime.h>
#include <config/config.h>

#include <stdint.h>
#include <bpf/bpf.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>

// Global variable definitions for accvm module
uintptr_t ctrl_base;
func_counter func_counters[1000];
int func_counter_idx;
trie_node *root;

// BPF file descriptors for accvm module
int vts_fd;
int bpf_sched_ctrl_fd;
int sim_proc_state_fd;

// System timing
uint64_t sys_up_time;

int vm_destroyed;
context_list* ctx_lst;
task_list* lst;
task_list* completed;
task_list* outlaw_lst;
task_list* sock_lst;
task_list* sock_outlaw_lst;
task_list_shared* shared_task_lst;
int fork_count;

// Function pointer definitions
int (*orig_nanosleep)(const struct timespec *req, struct timespec *rem);
int (*orig_clock_nanosleep)(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);
int (*orig_clock_gettime)(clockid_t, struct timespec *);
int (*orig_gettimeofday)(struct timeval *, void *);

int (*orig_pthread_mutex_lock)(pthread_mutex_t *mutex);
int (*orig_pthread_mutex_unlock)(pthread_mutex_t *mutex);
int (*orig_pthread_cond_wait)(pthread_cond_t *cond, pthread_mutex_t *mutex);
int (*orig_pthread_cond_signal)(pthread_cond_t *cond);

ssize_t (*orig_read)(int fd, void *buf, size_t count);
ssize_t (*orig_readv)(int fd, const struct iovec *iov, int iovcnt);
ssize_t (*orig_pread)(int fd, void *buf, size_t count, off_t offset);
ssize_t (*orig_pread64)(int fd, void *buf, size_t count, off_t offset);
ssize_t (*orig_pwrite)(int fd, const void *buf, size_t count, off_t offset);
ssize_t (*orig_pwrite64)(int fd, const void *buf, size_t count, off_t offset);
ssize_t (*orig_write)(int fd, const void *buf, size_t count);
ssize_t (*orig_writev)(int fd, const struct iovec *iov, int iovcnt);

int (*orig_accept4) (int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
int (*orig_epoll_wait)(int epfd, struct epoll_event *events, int maxevents, int timeout);
int (*orig_select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int (*orig_poll)(struct pollfd *fds, nfds_t nfds, int timeout);

ssize_t (*orig_recvfrom)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t (*orig_sendto)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t (*orig_send)(int sockfd, const void *buf, size_t len, int flags);
ssize_t (*orig_recv)(int sockfd, void *buf, size_t len, int flags);
int (*orig_listen)(int sockfd, int backlog);
int (*orig_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int (*orig_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int (*orig_execve)(const char *filename, char *const argv[], char *const envp[]);
int (*orig_execv)(const char *path, char *const argv[]);
int (*orig_execvp)(const char *file, char *const argv[]);
int (*orig_execvpe)(const char *file, char *const argv[], char *const envp[]);
int (*orig_execveat)(int dirfd, const char *pathname, char *const argv[], char *const envp[], int flags);
int (*orig_execl)(const char *path, const char *arg, ...);
int (*orig_execlp)(const char *file, const char *arg, ...);
int (*orig_execle)(const char *path, const char *arg, ...);

pid_t (*orig_fork)(void);
pid_t (*orig_vfork)(void);
pid_t (*orig_wait)(int *status);
pid_t (*orig_waitpid)(pid_t pid, int *status, int options);
pid_t (*orig_wait4) (pid_t pid, int *status, int options, struct rusage *rusage);

void (*orig_exit) (int status) __attribute__((noreturn));
void (*orig_exit_group) (int status) __attribute__((noreturn));

int (*orig_pthread_cond_timedwait)(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int (*orig_pthread_broadcast)(pthread_cond_t *cond);
int (*orig_pthread_create)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void *), void *);
int (*orig_clone)(int (*fn)(void *), void *child_stack, int flags, void *arg, ...);
int (*orig_pthread_detach)(pthread_t th);
int (*orig_pthread_join)(pthread_t th, void **thread_return);
int (*orig_pthread_join_np)(pthread_t th, void **thread_return);

int (*orig_pthread_mutex_trylock)(pthread_mutex_t *mutex);
int (*orig_pthread_rwlock_rdlock)(pthread_rwlock_t *rwlock);
int (*orig_pthread_rwlock_wrlock)(pthread_rwlock_t *rwlock);
int (*orig_pthread_rwlock_unlock)(pthread_rwlock_t *rwlock);
int (*orig_pthread_rwlock_tryrdlock)(pthread_rwlock_t *rwlock);
int (*orig_pthread_rwlock_trywrlock)(pthread_rwlock_t *rwlock);
int (*orig_sched_yield)(void);

int (*orig_sem_wait)(sem_t *sem);
int (*orig_sem_post)(sem_t *sem);
int (*orig_sem_timedwait)(sem_t *sem, const struct timespec *abs_timeout);

int (*orig_fsync)(int fd);

struct per_thread_state {
    __u64 halt_until;
};

#if CONFIG_ROUND_BASED_MODE
uint64_t read_vts(){
  __u32 index = 0;
  __u64 state = 0;
  __u64 vts = 0;
  bpf_map_lookup_elem(bpf_sched_ctrl_fd, &index, &state);
  if(state == 1){
    // printf("the quantum scheduling is on \n");
    __u32 index = 0;
    bpf_map_lookup_elem(vts_fd, &index, &vts);
  }else{
    // printf("the quantum scheduling is off \n");
    struct timespec ts;
    assert(orig_clock_gettime != NULL);
    orig_clock_gettime(CLOCK_MONOTONIC, &ts);
    vts = ts.tv_sec * 1000000000LL + ts.tv_nsec; 
  }
	return vts;
}
#endif

#if CONFIG_STOP_WORLD_MODE
uint64_t read_vts(){
	__u32 index = 0;
	__u64 vts = 0;
	bpf_map_lookup_elem(vts_fd, &index, &vts);
  struct timespec ts;
  if (orig_clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
      perror("clock_gettime");
      return -1;
  }
  PERF_INC_PURE(get_real_ts);
  uint64_t real = ts.tv_sec * 1000000000LL + ts.tv_nsec;
  // printf("vts %lu\n", vts);
	return real - vts;
}
#endif

int penalize_thread(int pid, uint64_t till){
  // printf("penalize thread %d till %lu\n", pid, till);
  // get state of pid 
  return 0;
}

uint64_t slowdown(){
  uint64_t elapsed = get_real_ts() - sys_up_time;
  return elapsed / read_vts();
}

// Wrapper around clock_gettime for concise code, in nanoseconds
uint64_t get_real_ts() {
  struct timespec ts;
  if (orig_clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
      perror("clock_gettime");
      return -1;
  }
  PERF_INC_PURE(get_real_ts);
  // self_ctx->vm_time += 21;
  return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Microseconds to timeval
void usToTimeval(uint64_t microseconds, struct timeval *time) {
  time->tv_sec = microseconds / 1000000;
  time->tv_usec = microseconds % 1000000;
}

#define TIME_MARGIN 1000 // 1 ms
void vm_entry(t_context *ctx, uint64_t *entry_ts) {
  *entry_ts = get_real_ts();
  // ctx->vm_time += COMPENSATE_TIME;
  uint64_t virtual_ts = *entry_ts - ctx->vm_time + COMPENSATE_TIME;
  if(virtual_ts/1000+TIME_MARGIN < ctx->virtual_ts/1000){
    printf("Thread<%lu>: vm entry time_us %lu; orig %lu\n", (unsigned long)(self_ctx->tid), virtual_ts/1000, ctx->virtual_ts/1000);
  }
  // 1000 us difference in time calculation
  assert(virtual_ts/1000+TIME_MARGIN >= ctx->virtual_ts/1000);
  ctx->virtual_ts = virtual_ts;// > ctx->virtual_ts ? virtual_ts : ctx->virtual_ts;
}

void vm_exit(t_context *ctx, uint64_t *entry_ts) {
  ctx->vm_time = get_real_ts() - self_ctx->virtual_ts + COMPENSATE_TIME;
}

void vm_adjust_vm_time(t_context *ctx, uint64_t *entry_ts){
  DEBUG("Thread<%lu>: adjust vm time with vts %lu\n", (unsigned long)(ctx->tid), ctx->virtual_ts/1000);
  *entry_ts = get_real_ts();
  ctx->vm_time = *entry_ts - ctx->virtual_ts+COMPENSATE_TIME;
}

// Preload overload of gettimeofday
int gettimeofday(struct timeval *tv, void *tz) {
  // printf("gettimeofday start\n");

  uint64_t vts = read_vts();
  usToTimeval(vts/1000, tv);
  // printf("gettimeofday %lu\n", vts/1000);
  return 0;
}

void ns_to_timespec(uint64_t ns, struct timespec *tp) {
  tp->tv_sec = ns / 1000000000;
  tp->tv_nsec = ns % 1000000000;
}

void ns_to_timeval(uint64_t ns, struct timeval *tv) {
  tv->tv_sec = ns / 1000000000;
  tv->tv_usec = (ns % 1000000000)/1000;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  if(clk_id == 999){
    orig_clock_gettime(CLOCK_MONOTONIC, tp);
  }else{
    uint64_t vts = read_vts();
    ns_to_timespec(vts, tp);
  }
  return 0;
}

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain) {
  printf("\n=== clock nanosleep\n");
  __u32 index2 = 1;
  __u64 state = 0;
  bpf_map_lookup_elem(bpf_sched_ctrl_fd, &index2, &state);
  if(state == 0){
    printf("clock nanosleep the quantum scheduling is off \n");
    return orig_clock_nanosleep(clock_id, flags, request, remain);
  }

  int thread_state_map_fd = bpf_obj_get("/sys/fs/bpf/thread_state_map");
  int pid = getpid();
  uint64_t start_t = read_vts();
  uint64_t orig_sleep_time = request->tv_sec*1000000000 + request->tv_nsec;
  struct per_thread_state thread_state;
  thread_state.halt_until = start_t + orig_sleep_time;
  int ret = bpf_map_update_elem(thread_state_map_fd, &pid, &thread_state, BPF_ANY);
  return ret;
}


int nanosleep(const struct timespec *req, struct timespec *rem) {
  printf("\n=== nanosleep\n");
  uint64_t start_t = read_vts();
  uint64_t orig_sleep_time = req->tv_sec*1000000000 + req->tv_nsec;
  uint64_t end_t = start_t + orig_sleep_time;
  uint64_t time_now = start_t;
  while(time_now < end_t){
    orig_nanosleep(req, rem);
    time_now = read_vts();
  }
  return 0;
}

