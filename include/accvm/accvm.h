#ifndef ACCVM_H__
#define ACCVM_H__
#include <config/config.h>
#include <accvm/context.h>
#include <accvm/thread.h>
#include <accvm/perf.h>
// #define _GNU_SOURCE
// #define __USE_GNU
#include <stdint.h>
#include <sys/ucontext.h>
#include <capstone/capstone.h>
#include <unistd.h>

extern uintptr_t ctrl_base;

#if CONFIG_PERF
  #define PERF_INC_PURE(func_name) perf_incr_func_counter(#func_name);
  #define PERF_INC(func_name) uint64_t start_vts = self_ctx->virtual_ts; perf_incr_func_counter(#func_name);
  #define PERF_INC_TIME(func_name, end_vts) perf_incr_func_time(#func_name, start_vts, end_vts);
#else
  #define PERF_INC_PURE(func_name)
  #define PERF_INC(func_name) 
  #define PERF_INC_TIME(func_name, end_vts)
#endif

#if CONFIG_DEBUG_HARMLESS
    #define DEBUG(...) printf(__VA_ARGS__)
#else
    #define DEBUG(...)
#endif

#if CONFIG_DEBUG_HARM
    #define DEBUG_H(...) printf(__VA_ARGS__)
#else
    #define DEBUG_H(...)
#endif

#if CONFIG_DEBUG_MMIO
    #define DEBUG_MMIO(...) printf(__VA_ARGS__)
#else
    #define DEBUG_MMIO(...)
#endif


#define VM_EXIT(self_ctx)\
  DEBUG("atexit task_size %d, outlaw size %d, completed size %d\n", get_size(lst), get_size(outlaw_lst), get_size(completed));\
  self_ctx->freeze_vts = 0; \
  vm_exit(self_ctx, &entry_ts);

// no sync
#define VM_ENTRY_NS(self_ctx)\
  uint64_t entry_ts; \
  vm_entry(self_ctx, &entry_ts); \

#define PRINT_SIZE(lst, prepend, outlaw_lst)\
  DEBUG("PRINT:" #prepend " lst ids: "); \
  int a = get_size(lst); \
  DEBUG("PRINT: " #prepend " outlaw_lst ids: "); \
  int b = get_size(outlaw_lst); \
  DEBUG("PRINT: completed ids: "); \
  int c = get_size(completed); \
  DEBUG("atenter task_size %d, outlaw size %d, completed size %d\n", a,b,c);\

#define OUTLAW_COMM_UPDATE_VM_TIME(out_lst)  \
    entry_ts = get_real_ts(); \
    orig_sem_wait(&out_lst->sem); \
    self_ctx->freeze_vts = 1; \
    self_ctx->vts_updated = 0; \
    orig_sem_post(&out_lst->sem); \
  
#define OUTLAW_COMM_UPDATE_VM_TIME_TIMEOUT(out_lst)  \
    entry_ts = get_real_ts(); \
    orig_sem_wait(&out_lst->sem); \
    self_ctx->freeze_vts = 1; \
    if (self_ctx->vts_updated == 1) \
    { \
      PERF_INC(outlaw_updated);\
      self_ctx->vts_updated = 0; \
      PERF_INC_TIME(outlaw_updated, self_ctx->virtual_ts); \
    }else{\
      self_ctx->virtual_ts = entry_ts - self_ctx->vm_time; \
    } \
    orig_sem_post(&out_lst->sem); \

#define VM_ENTRY(lst, self_ctx) \
  uint64_t entry_ts; \
  if (self_ctx->outlaw == true) \
  { \
    OUTLAW_COMM_UPDATE_VM_TIME(outlaw_lst); \
  } \
  else{ \
    vm_entry(self_ctx, &entry_ts); \
    vts_barrier(lst, outlaw_lst, self_ctx); \
  }

#define VM_SOCK_ENTRY(lst, self_ctx) \
  uint64_t entry_ts; \
  if (self_ctx->outlaw == true) \
  { \
    OUTLAW_COMM_UPDATE_VM_TIME(sock_outlaw_lst); \
  } \
  else{ \
    vm_entry(self_ctx, &entry_ts); \
    task* taskp = get_task(lst);  \
    taskp->ctx_idx = self_ctx->ctx_idx; \
    taskp = get_task(sock_outlaw_lst);  \
    taskp->ctx_idx = self_ctx->ctx_idx; \
    vts_barrier(lst, sock_outlaw_lst, self_ctx); \
  }

#define VM_ENTRY_TIMEOUT(lst, self_ctx) \
  uint64_t entry_ts; \
  if (self_ctx->outlaw == true) \
  { \
    OUTLAW_COMM_UPDATE_VM_TIME_TIMEOUT(outlaw_lst); \
  } \
  else{ \
    vm_entry(self_ctx, &entry_ts); \
    vts_barrier(lst, outlaw_lst, self_ctx); \
  }

#define VM_SOCK_ENTRY_TIMEOUT(lst, self_ctx) \
  uint64_t entry_ts; \
  if (self_ctx->outlaw == true) \
  { \
    OUTLAW_COMM_UPDATE_VM_TIME_TIMEOUT(sock_outlaw_lst); \
  } \
  else{ \
    vm_entry(self_ctx, &entry_ts); \
    task* taskp = get_task(lst);  \
    taskp->ctx_idx = self_ctx->ctx_idx; \
    taskp = get_task(sock_outlaw_lst);  \
    taskp->ctx_idx = self_ctx->ctx_idx; \
    vts_barrier(lst, sock_outlaw_lst, self_ctx); \
  }

#define SCALETIME 1 // 1:1 time scale
#define MMIO_SIZE 4096*2

#define DECL_OUTLAW(data_struct, dst_state) data_struct = dst_state;

void set_mmio_to_user();
#endif