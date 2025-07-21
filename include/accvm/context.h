#ifndef ACCVM_CONTEXT_H
#define ACCVM_CONTEXT_H
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>
#include <accvm/func_tags.h>
#include <sys/types.h>

#define FORCE_WAIT_FOR_OUTLAW 1000000 // 1 ms
extern int vm_destroyed;

// Mutex lock
extern int (*orig_pthread_mutex_lock)(pthread_mutex_t *mutex);
extern int (*orig_pthread_mutex_unlock)(pthread_mutex_t *mutex);
extern int (*orig_pthread_cond_wait)(pthread_cond_t *cond, pthread_mutex_t *mutex);
extern int (*orig_pthread_cond_signal)(pthread_cond_t *cond);

#define MAX_TASKS 1000
#define NO_NEXT -1 // Indicates no next task
#define AS(list, idx) list->tasks[idx]
#define AS_CTX(list, idx) ctx_lst->arr_ctx[list->tasks[idx].ctx_idx]
#define G_CTX(idx) ctx_lst->arr_ctx[idx]
#define COND_WAIT(cond, lock) \
  orig_sem_post(lock);\
  orig_sem_wait(cond);\
  orig_sem_wait(lock);\

#define REVERT_WINDOW 100
#define VERSION_WINDOW 10
#define DIFF_TOLERANCE 100000

typedef struct {
  pid_t tid;
  pid_t pid;
  pthread_t pthread_id;
  bool outlaw;
  bool pend_sock;
  int func_tag;
  int vts_updated;
  int freeze_vts;
  uint64_t vm_time;
  uint64_t virtual_ts;
  sem_t wait;
  int ctx_idx;

  // for reverting time
  int force_revert;
  uint64_t revert_vts[REVERT_WINDOW];
  int r_app_indx;

  // for fork
  int vfork_c;
} t_context;

typedef struct {
  int version;
  int inlist;
  int ctx_idx;
  int next;        // Offset of the next task in the array (or NO_NEXT)
} task;

typedef struct{
  int version;
  uint64_t vts;
} head_version;

typedef struct {
  char name;
  int init_done;
  sem_t sem;
  int tail; // Offset of the tail of the used memory
  int head; // Offset of the head of the list
  task tasks[MAX_TASKS];

  // lst head version control
  int head_v_idx;
  head_version head_v_arr[VERSION_WINDOW];
  uint64_t head_v;
} task_list;

typedef struct{
  int init_done;
  sem_t sem;
  int tail;
  t_context arr_ctx[MAX_TASKS];
} context_list; 

extern context_list* ctx_lst;
// Safe, sorted linked list
extern task_list* lst;
extern task_list* completed;
extern task_list* outlaw_lst;

extern task_list* sock_lst;
extern task_list* sock_outlaw_lst;

extern void *mmio_base;


// Thread local context variable
extern __thread t_context *self_ctx;

int get_size(task_list *lst);
// Create and insert a task to the list
void schedule(task_list *lst, t_context *ctx);

// Deschedules a task, returns a pointer to context
t_context *deschedule(task_list *lst, pid_t tid, int wakeup_head);
t_context *deschedule_tid(task_list *lst, pthread_t tid);
t_context *deschedule_pid(task_list *lst, pid_t pid);

// Yield to the running task
void yield(task_list *lst, t_context *ctx);

// Syncs all threads to context's virtual timestamp
void sync_t(task_list *lst, t_context *ctx);

void wait_on(task_list *lst, pid_t tid);
void wait_on_next(task_list *lst);
void mv_vts_to_next(task_list *lst);
void vts_barrier(task_list *lst, task_list *outlaw_lst, t_context *ctx);
void clear_main(task_list *lst);

task_list *init_task_list(const char *shm_name, int init, int* fd);
context_list *init_context(const char *shm_name, int init, int* fd);
void* init_mmio_region(const char *shm_name, int size, int init, int* fd);

void deinit_mmio_region(void* mmio_base, int size, int fd);
void deinit_context(context_list *lst, int fd);
void deinit_task_list(task_list *lst, int fd);

task* get_new_task(task_list *lst);
task* get_old_task(task_list *lst);
task* get_task(task_list* lst);
int exist_task(task_list* lst, pid_t pid);
int get_new_ctx(context_list *lst);
t_context* get_old_ctx(context_list *lst, pid_t tid);

void initialize_functions();
#endif