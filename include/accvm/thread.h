#ifndef ACCVM_THREAD_H
#define ACCVM_THREAD_H

#include <accvm/acctime.h>
#include <accvm/accvm.h>
#include <accvm/context.h>
#include <bits/pthreadtypes.h>
#include <bits/types/siginfo_t.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>  // For SYS_gettid
#include <sys/resource.h>

extern int (*orig_execve)(const char *filename, char *const argv[], char *const envp[]);
extern int (*orig_execv)(const char *path, char *const argv[]);
extern int (*orig_execvp)(const char *file, char *const argv[]);
extern int (*orig_execvpe)(const char *file, char *const argv[], char *const envp[]);
extern int (*orig_execveat)(int dirfd, const char *pathname, char *const argv[], char *const envp[], int flags);
extern int (*orig_execl)(const char *path, const char *arg, ...);
extern int (*orig_execlp)(const char *file, const char *arg, ...);
extern int (*orig_execle)(const char *path, const char *arg, ...);

pid_t gettid();

// process
extern pid_t (*orig_fork)(void);
extern pid_t (*orig_vfork)(void);


extern pid_t (*orig_wait)(int *status);
extern pid_t (*orig_waitpid)(pid_t pid, int *status, int options);
extern pid_t (*orig_wait4) (pid_t pid, int *status, int options, struct rusage *rusage);

extern void (*orig_exit) (int status) __attribute__((noreturn));
extern void (*orig_exit_group) (int status) __attribute__((noreturn));

// thread
extern int (*orig_pthread_cond_wait)(pthread_cond_t *cond, pthread_mutex_t *mutex);
extern int (*orig_pthread_cond_timedwait)(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
extern int (*orig_pthread_cond_signal)(pthread_cond_t *cond);
extern int (*orig_pthread_broadcast)(pthread_cond_t *cond);

extern int (*orig_pthread_create)(pthread_t *, const pthread_attr_t *,
                           void *(*start_routine)(void *), void *);
extern int (*orig_clone)(int (*fn)(void *), void *child_stack, int flags, void *arg, ...);
extern int (*orig_pthread_detach)(pthread_t th);
extern int (*orig_pthread_join)(pthread_t th, void **thread_return);
extern int (*orig_pthread_join_np)(pthread_t th, void **thread_return);

extern int (*orig_pthread_mutex_lock)(pthread_mutex_t *mutex);
extern int (*orig_pthread_mutex_trylock)(pthread_mutex_t *mutex);
extern int (*orig_pthread_mutex_unlock)(pthread_mutex_t *mutex);
extern int (*orig_pthread_rwlock_rdlock)(pthread_rwlock_t *rwlock);
extern int (*orig_pthread_rwlock_wrlock)(pthread_rwlock_t *rwlock);
extern int (*orig_pthread_rwlock_unlock)(pthread_rwlock_t *rwlock);
extern int (*orig_pthread_rwlock_tryrdlock)(pthread_rwlock_t *rwlock);
extern int (*orig_pthread_rwlock_trywrlock)(pthread_rwlock_t *rwlock);
extern int (*orig_sched_yield)(void);

extern int (*orig_sem_wait)(sem_t *sem);
extern int (*orig_sem_post)(sem_t *sem);
extern int (*orig_sem_timedwait)(sem_t *sem, const struct timespec *abs_timeout);

// Struct for wrapping pthread arguments
typedef struct {
  pthread_t *tid;
  const pthread_attr_t *attr;
  void *(*func)(void *);
  void *arg;
  t_context* ctx;
} t_arg;

#define FORKED_NUM_PARTIES 1000
typedef struct {
    task_list* lst[FORKED_NUM_PARTIES];
    bool* valid[FORKED_NUM_PARTIES];
}task_list_shared;

extern task_list_shared* shared_task_lst;

extern int fork_count;

// Initialize context for threads
void *thread_init(void *args);

// Initialize context for main thread
void main_init();

// Wrap pthread_create function
int pthread_create(pthread_t *newthread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);

// Wrap pthread_join, which syncs only with child thread
int pthread_join(pthread_t th, void **thread_return);

int pthread_mutex_lock(pthread_mutex_t *mutex);

int pthread_mutex_unlock(pthread_mutex_t *mutex);

void thread_decl_outlaw(bool outlaw, task_list* the_list, task_list* outlaw_lst);

#define OVER_WRITE_OUTLAW(func_name, ret_type, tag_for_func, ...) \
  DEBUG("Thread<%lu>: try enter " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  self_ctx->func_tag = tag_for_func; \
  VM_ENTRY(lst, self_ctx); \
  PERF_INC(func_name); \
  DEBUG("Thread<%lu>: enter " #func_name " vts %lu \n", (unsigned long)(self_ctx->tid), self_ctx->virtual_ts/1000000); \
  bool toggle_to; \
  if (self_ctx->outlaw == true) {assert(0);toggle_to = true;} \
  else {toggle_to = false;} \
  thread_decl_outlaw(true, lst, outlaw_lst); \
  VM_EXIT(self_ctx); \
  DEBUG("Thread<%lu>: calling " #func_name " vts %lu \n", (unsigned long)(self_ctx->tid), self_ctx->virtual_ts/1000000); \
  ret_type ret = orig_##func_name(__VA_ARGS__); \
  { \
  DEBUG("Thread<%lu>: after calling " #func_name " vts %lu \n", (unsigned long)(self_ctx->tid), self_ctx->virtual_ts/1000000); \
  VM_ENTRY(lst, self_ctx); \
  thread_decl_outlaw(toggle_to, lst, outlaw_lst); \
  self_ctx->func_tag = 0; \
  self_ctx->outlaw = false; \
  DEBUG("Thread<%lu>: success " #func_name " vts %lu \n", (unsigned long)(self_ctx->tid), self_ctx->virtual_ts/1000000); \
  VM_EXIT(self_ctx);\
  } \
  PERF_INC_TIME(func_name, self_ctx->virtual_ts); \
  return ret; \

#define OVER_WRITE_OUTLAW_NORET(func_name, ret_type, tag_for_func, ...) \
  DEBUG("Thread<%lu>: try enter " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  self_ctx->func_tag = tag_for_func; \
  VM_ENTRY(lst, self_ctx); \
  PERF_INC(func_name); \
  DEBUG("Thread<%lu>: enter " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  bool toggle_to; \
  if (self_ctx->outlaw == true) {toggle_to = true;} \
  else {toggle_to = false;} \
  thread_decl_outlaw(true, lst, outlaw_lst); \
  VM_EXIT(self_ctx); \
  DEBUG("Thread<%lu>: calling " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  ret_type ret = orig_##func_name(__VA_ARGS__); \
  DEBUG("Thread<%lu>: after calling " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  { \
  VM_ENTRY(lst, self_ctx); \
  thread_decl_outlaw(toggle_to, lst, outlaw_lst); \
  self_ctx->func_tag = 0; \
  DEBUG("Thread<%lu>: success " #func_name " outlaw %d \n", (unsigned long)(self_ctx->tid), toggle_to); \
  VM_EXIT(self_ctx);\
  } \
  PERF_INC_TIME(func_name, self_ctx->virtual_ts); \

#define OVER_WRITE_OUTLAW_NORET_TIMEOUT(func_name, ret_type, tag_for_func, ...) \
  DEBUG("Thread<%lu>: try enter " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  self_ctx->func_tag = tag_for_func; \
  VM_ENTRY_TIMEOUT(lst, self_ctx); \
  PERF_INC(func_name); \
  DEBUG("Thread<%lu>: enter " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  bool toggle_to; \
  if (self_ctx->outlaw == true) {toggle_to = true;} \
  else {toggle_to = false;} \
  thread_decl_outlaw(true, lst, outlaw_lst); \
  VM_EXIT(self_ctx); \
  DEBUG_H("Thread<%lu>: calling " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  ret_type ret = orig_##func_name(__VA_ARGS__); \
  DEBUG_H("Thread<%lu>: after calling " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  { \
  VM_ENTRY_TIMEOUT(lst, self_ctx); \
  thread_decl_outlaw(toggle_to, lst, outlaw_lst); \
  DEBUG("Thread<%lu>: exit thread_decl_outlaw " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  self_ctx->func_tag = 0; \
  DEBUG("Thread<%lu>: success " #func_name " outlaw %d \n", (unsigned long)(self_ctx->tid), toggle_to); \
  VM_EXIT(self_ctx);\
  } \
  PERF_INC_TIME(func_name, self_ctx->virtual_ts); \

#define OVER_WRITE_OUTLAW_TIMEOUT(func_name, ret_type, tag_for_func, abs_time, timeoutval, ...) \
  while(1){ \
    OVER_WRITE_OUTLAW_NORET_TIMEOUT(func_name, ret_type, tag_for_func, __VA_ARGS__); \
    if (ret == timeoutval && self_ctx->virtual_ts < abs_time) { \
      continue; \
    } { \
      if (ret == timeoutval) { \
        self_ctx->virtual_ts = abs_time; \
        self_ctx->vm_time = get_real_ts() - self_ctx->virtual_ts + COMPENSATE_TIME; \
      } \
      return ret; \
    } \
  } \

#define OVER_WRITE(func_name, ret_type, tag_for_func, ...) \
  DEBUG("Thread<%lu>: try enter " #func_name "\n", (unsigned long)(self_ctx->tid)); \
  self_ctx->func_tag = tag_for_func; \
  VM_ENTRY(lst, self_ctx); \
  PERF_INC(func_name); \
  assert(self_ctx->outlaw == false); \
  DEBUG("Thread<%lu>: enter " #func_name " outlaw %d \n", (unsigned long)(self_ctx->tid), self_ctx->outlaw); \
  VM_EXIT(self_ctx); \
  ret_type ret = orig_##func_name(__VA_ARGS__); \
  PERF_INC_TIME(func_name, self_ctx->virtual_ts); \
  self_ctx->func_tag = 0; \
  return ret; \
 
#define NEW_THREAD(thread_local_ctx) \
  int ctx_idx = get_new_ctx(ctx_lst); \
  task* new_task = get_new_task(lst); \
  new_task->ctx_idx = ctx_idx; \
  task* new_task2 = get_new_task(outlaw_lst); \
  new_task2->ctx_idx = ctx_idx;  \
  task* new_task3 = get_new_task(completed);  \
  new_task3->ctx_idx = ctx_idx; \
  thread_local_ctx = &ctx_lst->arr_ctx[ctx_idx]; \
  thread_local_ctx->ctx_idx = ctx_idx; \


#endif