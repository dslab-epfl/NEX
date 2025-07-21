#include <accvm/acctime.h>
#include <accvm/func_tags.h>
#include <accvm/context.h>
#include <accvm/thread.h>
#include <accvm/network.h>
#include <accvm/fileops.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

__thread t_context *self_ctx;

t_context* get_old_ctx(context_list *lst, pid_t tid) {
    DEBUG_H("Thread<%lu>: enter get old context; tail length %d,lock %p\n", (unsigned long)(gettid()), lst->tail, &lst->sem);
    int new_task_index = -1;
    orig_sem_wait(&lst->sem);
    for (int i = 0; i < lst->tail; i++) {
        if (lst->arr_ctx[i].tid == tid) {
            // reuse my previou slots
            new_task_index = i;
            // the thread is currently invalid in the list
            DEBUG_H("Thread<%lu>: get old context %d\n", (unsigned long)(tid), new_task_index);
            break;
        }
    }
    orig_sem_post(&lst->sem);
    if(new_task_index == -1) {
        fprintf(stderr, "No old task slots available.\n");
        return NULL;
        assert(0);
    }
  return &lst->arr_ctx[new_task_index];
}

int get_new_ctx(context_list *lst) {
  int id = 0;
  orig_sem_wait(&lst->sem);
  id = lst->tail++;
  if(lst->tail == MAX_TASKS) {
    fprintf(stderr, "No free task slots available.\n");
    orig_sem_post(&lst->sem);
    assert(0);
  }
  orig_sem_post(&lst->sem); 
  return id;
}

task* get_new_task(task_list *lst) {
  int id = 0;
  orig_sem_wait(&lst->sem);
  id = lst->tail++;
  if(lst->tail == MAX_TASKS) {
    fprintf(stderr, "No free task slots available.\n");
    orig_sem_post(&lst->sem);
    assert(0);
  }
  orig_sem_post(&lst->sem); 
  return &lst->tasks[id];
}

task* get_task(task_list* list){
    int new_task_index = -1;
    pid_t tid = gettid();
    orig_sem_wait(&list->sem);
    for (int i = 0; i < list->tail; i++) {
        if (AS_CTX(list, i).tid == tid) {
            // reuse my previou slots
            new_task_index = i;
            // the thread is currently invalid in the list
            DEBUG_H("Thread<%lu>: which list %s, get old task %d, status %d\n", (unsigned long)(tid), &list->name, new_task_index, list->tasks[new_task_index].inlist);
            break;
        }
    }
    orig_sem_post(&list->sem);
    if(new_task_index == -1) {
        DEBUG_H("Thread<%lu>: which list %s, get new task %d\n", (unsigned long)(tid), &list->name, list->tail);
        return get_new_task(list);
    }
    return &list->tasks[new_task_index];
}

int exist_task(task_list* lst, pid_t pid){
    orig_sem_wait(&lst->sem);
    int current_index = lst->head;
    int ans = 0;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).pid != pid) {
        current_index = lst->tasks[current_index].next;
    }
    if (current_index != NO_NEXT) {
        ans = AS_CTX(lst, current_index).pid == pid && lst->tasks[current_index].inlist == 1;
    }
    orig_sem_post(&lst->sem);
    return ans;
}
task* get_old_task(task_list* list){
    int new_task_index = -1;
    pid_t tid = gettid();
    orig_sem_wait(&list->sem);
    for (int i = 0; i < list->tail; i++) {
        if (AS_CTX(list, i).tid == tid) {
            // reuse my previou slots
            new_task_index = i;
            // the thread is currently invalid in the list
            DEBUG_H("Thread<%lu>: which list %s, get old task %d, status %d\n", (unsigned long)(tid), &list->name, new_task_index, list->tasks[new_task_index].inlist);
            break;
        }
    }
    orig_sem_post(&list->sem);
    if(new_task_index == -1) {
        return NULL;
    }
    return &list->tasks[new_task_index];
}


int create_smem(const char *shm_name, int size, int init) {
    int shm_fd;
    if (init == 0) {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        DEBUG("Open/not create %s shm_fd %d\n", shm_name, shm_fd);
        if (shm_fd == -1) {
            perror("shm_open");
            return -1;
        }
    }else{
        shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open");
            return -1;
        }
        if (ftruncate(shm_fd, size) == -1) {
            perror("ftruncate");
            return -1;
        }
    }
    return shm_fd;
}


void* init_mmio_region(const char *shm_name, int size, int init, int* fd){
    int shm_fd = create_smem(shm_name, size, init);
    *fd = shm_fd;
    void* mmio_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mmio_base == MAP_FAILED) {
        perror("Failed to map MMIO region");
        exit(EXIT_FAILURE);
    }
    if (init == 0) {
        mprotect(mmio_base, size, PROT_NONE); // Remove protection
    }else{
        memset(mmio_base, 0, size);
        mprotect(mmio_base, size, PROT_NONE); // Remove protection
    }
    return mmio_base;
}

context_list *init_context(const char *shm_name, int init, int* fd) {
    int shm_fd = create_smem(shm_name, sizeof(context_list), init);
    *fd = shm_fd;
    context_list* ctx_lst = mmap(NULL, sizeof(context_list), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    if (ctx_lst == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    if (init == 0) {
        return ctx_lst;
    }
    memset(ctx_lst, 0, sizeof(context_list));
    ctx_lst->init_done = 1;
    sem_init(&ctx_lst->sem, 1, 1);

    for (int i = 0; i < MAX_TASKS; i++) {
        t_context *ctx = &ctx_lst->arr_ctx[i];
        ctx->tid = 0;
        ctx->pid = 0;
        ctx->pthread_id = 0;
        ctx->outlaw = 0;
        ctx->func_tag = 0;
        ctx->vts_updated = 0;
        ctx->freeze_vts = 0;
        ctx->vm_time = 0;
        ctx->virtual_ts = 0;
        sem_init(&ctx->wait, 1, 0);
        ctx->ctx_idx = -1;
        ctx->force_revert = -1;
        ctx->r_app_indx = 0;
        memset(ctx->revert_vts, 0, sizeof(uint64_t)*REVERT_WINDOW);
        ctx->vfork_c = 0;
    }
    return ctx_lst;
}

task_list *init_task_list(const char *shm_name, int init, int* fd) {
    int shm_fd = create_smem(shm_name, sizeof(context_list), init);
    *fd = shm_fd;
    task_list *list = mmap(NULL, sizeof(task_list), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (list == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    if (init == 0) {
        return list;
    }

    memset(list, 0, sizeof(task_list));
  
    list->init_done = 1;
    sem_init(&list->sem, 1, 1);
    list->head = NO_NEXT;
    list->tail = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        list->tasks[i].ctx_idx = NO_NEXT;
        list->tasks[i].next = NO_NEXT;
        list->tasks[i].inlist = 0;
    }
    list->head_v_idx = 0;
    list->head_v = 0;
    memset(list->head_v_arr, 0, sizeof(head_version)*VERSION_WINDOW);
    return list;
}

void deinit_mmio_region(void* mmio_base, int size, int fd){
    munmap(mmio_base, size);
    close(fd);
}

void deinit_context(context_list *lst, int fd) {
    munmap(lst, sizeof(context_list));
    close(fd);
}

void deinit_task_list(task_list *lst, int fd) {
    munmap(lst, sizeof(task_list));
    close(fd);
}

int get_size(task_list *list) {
    int size = 0;
    orig_sem_wait(&list->sem);
    int current_index = list->head;
    while (current_index != NO_NEXT) {
        size++;
        if (list == lst)
            DEBUG("S T<%d> ", AS_CTX(list, current_index).tid);
        current_index = list->tasks[current_index].next;
    }
    if (list == lst)
        DEBUG("\n");
    orig_sem_post(&list->sem);
    return size;
}

// ctx points to the same location as the task_list
void schedule(task_list *lst, t_context *ctx) {
    orig_sem_wait(&lst->sem);

    int new_task_index = -1;
    for (int i = 0; i < lst->tail; i++) {
        if (AS_CTX(lst, i).tid == ctx->tid) {
            // reuse my previou slots
            new_task_index = i;
            // the thread is currently invalid in the list
            if (lst->tasks[new_task_index].inlist == 1)
                printf("Thread<%lu>: reuse the slot %d\n", (unsigned long)(ctx->tid), new_task_index);
            assert(lst->tasks[new_task_index].inlist == 0);
            break;
        }
    }

    if (new_task_index == -1){
        // this is a new thread, it should get_new_idx first;
        assert(0);
    }

    lst->tasks[new_task_index].inlist = 1;
    lst->tasks[new_task_index].next = NO_NEXT;
    lst->tasks[new_task_index].ctx_idx = ctx->ctx_idx;

    int prev_index = NO_NEXT;
    int current_index = lst->head;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).virtual_ts <= ctx->virtual_ts) {
        prev_index = current_index;
        current_index = lst->tasks[current_index].next;
    }

    // Insert the new task into the list
    lst->tasks[new_task_index].next = current_index;
    if (prev_index == NO_NEXT) {
        lst->head = new_task_index;
    } else {
        lst->tasks[prev_index].next = new_task_index;
    }

    orig_sem_post(&lst->sem);
}

t_context *deschedule(task_list *lst, pid_t tid, int wakeup_head) {
    orig_sem_wait(&lst->sem);
    int prev_index = NO_NEXT;
    int current_index = lst->head;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).tid != tid) {
        prev_index = current_index;
        current_index = lst->tasks[current_index].next;
    }
    if (current_index != NO_NEXT) {
        if (prev_index == NO_NEXT) {
            lst->head = lst->tasks[current_index].next;
        } else {
            lst->tasks[prev_index].next = lst->tasks[current_index].next;
        }

        lst->tasks[current_index].next = NO_NEXT;
        lst->tasks[current_index].inlist = 0;
        
        if (wakeup_head && lst->head != NO_NEXT) {
       
            orig_sem_post(&AS_CTX(lst, lst->head).wait);
        }

        orig_sem_post(&lst->sem);
        return &AS_CTX(lst, current_index);
    }

    if (wakeup_head && lst->head != NO_NEXT) {
        orig_sem_post(&AS_CTX(lst, lst->head).wait);
    }
    orig_sem_post(&lst->sem);
    return NULL;
}

void reschedule(task_list* lst, t_context* ctx){
    orig_sem_wait(&lst->sem);
    // deschedule
    int prev_index = NO_NEXT;
    int current_index = lst->head;
    int new_task_index = -1;

    while (current_index != NO_NEXT && AS_CTX(lst, current_index).tid != ctx->tid) {
        prev_index = current_index;
        current_index = lst->tasks[current_index].next;
    }
    if (current_index != NO_NEXT) {
        if (prev_index == NO_NEXT) {
            lst->head = lst->tasks[current_index].next;
            if (lst->head != NO_NEXT)
                orig_sem_post(&AS_CTX(lst, lst->head).wait);
        } else {
            lst->tasks[prev_index].next = lst->tasks[current_index].next;
        }
        lst->tasks[current_index].next = NO_NEXT;
        lst->tasks[current_index].inlist = 0;
        new_task_index = current_index;
    } else{
        // not in the list
        for (int i = 0; i < lst->tail; i++) {
            if (AS_CTX(lst, i).tid == ctx->tid) {
                // reuse my previou slots
                new_task_index = i;
                // the thread is currently invalid in the list
                // printf("Thread<%lu>: reuse the slot %d, status %d\n", (unsigned long)(ctx->tid), new_task_index, lst->valid[new_task_index]);
                if (lst->tasks[new_task_index].inlist == 1) {
                    printf("Fault it's should not be in the list: Thread<%lu>: reuse the slot %d\n", (unsigned long)(ctx->tid), new_task_index);
                    assert(0);
                }
                break;
            }
        }
    }

    // schedule
    if (new_task_index == -1){
        // this is a new thread, it should get_new_idx first;
        assert(0);
    }

    lst->tasks[current_index].inlist = 1;
    lst->tasks[new_task_index].next = NO_NEXT;
    lst->tasks[new_task_index].ctx_idx = ctx->ctx_idx;

    prev_index = NO_NEXT;
    current_index = lst->head;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).virtual_ts <= ctx->virtual_ts) {
        prev_index = current_index;
        current_index = lst->tasks[current_index].next;
    }

    // Insert the new task into the list
    lst->tasks[new_task_index].next = current_index;
    if (prev_index == NO_NEXT) {
        lst->head = new_task_index;
    } else {
        lst->tasks[prev_index].next = new_task_index;
    }
    orig_sem_post(&lst->sem);
}

t_context *deschedule_tid(task_list *lst, pthread_t tid) {
    orig_sem_wait(&lst->sem);
    int prev_index = NO_NEXT;
    int current_index = lst->head;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).pthread_id != tid) {
        prev_index = current_index;
        current_index = lst->tasks[current_index].next;
    }
    if (current_index != NO_NEXT) {
        if (prev_index == NO_NEXT) {
            lst->head = lst->tasks[current_index].next;
        } else {
            lst->tasks[prev_index].next = lst->tasks[current_index].next;
        }
        // reset the node
        lst->tasks[current_index].next = NO_NEXT;
        lst->tasks[current_index].inlist = 0;
        orig_sem_post(&lst->sem);
        return &AS_CTX(lst, current_index);
    }
    orig_sem_post(&lst->sem);
    return NULL;
}

t_context *deschedule_pid(task_list *lst, pid_t pid) {
    orig_sem_wait(&lst->sem);
    int prev_index = NO_NEXT;
    int current_index = lst->head;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).pid != pid) {
        prev_index = current_index;
        current_index = lst->tasks[current_index].next;
    }
    if (current_index != NO_NEXT) {
        if (prev_index == NO_NEXT) {
            lst->head = lst->tasks[current_index].next;
        } else {
            lst->tasks[prev_index].next = lst->tasks[current_index].next;
        }
        // reset the node
        lst->tasks[current_index].next = NO_NEXT;
        lst->tasks[current_index].inlist = 0;
        orig_sem_post(&lst->sem);
        return &AS_CTX(lst, current_index);
    }
    orig_sem_post(&lst->sem);
    return NULL;
}

// Yield to the running task
void yield(task_list *lst, t_context *ctx) {
  orig_sem_wait(&lst->sem);
  
  assert(lst->head != NO_NEXT);
  
  while (lst->head != NO_NEXT && AS_CTX(lst, lst->head).tid != self_ctx->tid) {
    // revert time here
    // Because I will go sleep soon, let me check against all versions of the head
#if 0
    if(self_ctx->force_revert == -1){

        // find my index
        int my_index = lst->head;
        while(AS_CTX(lst, my_index).tid != self_ctx->tid){
            my_index = AS(lst, my_index).next;
        }
        assert(my_index != NO_NEXT);
        int my_version = AS(lst, my_index).version;
        uint64_t min_value_to_revert = self_ctx->virtual_ts;
        for (int i = 0; i < VERSION_WINDOW; i++) {
            if (lst->head_v_arr[i].version <= my_version) {
                // older version, don't care
                continue;
            }
            uint64_t check_vts = lst->head_v_arr[i].vts;
            // new version, check if I shouldn't run, i.e. my vts is larger than the new version
            for (int j = 0; j < REVERT_WINDOW; j++) {
                if (self_ctx->revert_vts[j] != 0 && check_vts < self_ctx->revert_vts[j]) {
                    if ( min_value_to_revert > self_ctx->revert_vts[j]){
                        min_value_to_revert = self_ctx->revert_vts[j];
                        self_ctx->force_revert = j;
                    }
                }
            }
            // break;
        }
    }
#endif 
    COND_WAIT(&self_ctx->wait, &lst->sem);
  }

#if 0
  if(self_ctx->force_revert > -1){
    // this is safer because I know I am at the head now, reduce vts doesn't affect anything after me.
    DEBUG("Thread<%lu>: idx %d revert time from %lu to %lu\n", (unsigned long)(self_ctx->tid), self_ctx->force_revert ,self_ctx->virtual_ts/1000, self_ctx->revert_vts[self_ctx->force_revert]/1000);
    self_ctx->virtual_ts = self_ctx->revert_vts[self_ctx->force_revert];
    self_ctx->force_revert = -1;

  }else{
    int insrt = 1;
    for(int i = 0; i < REVERT_WINDOW; i++){
        if (
            self_ctx->revert_vts[i] - self_ctx->virtual_ts < DIFF_TOLERANCE || 
            self_ctx->virtual_ts - self_ctx->revert_vts[i] < DIFF_TOLERANCE ){
                insrt = 0;
                break;

        }
    }
    if (insrt == 1){
        self_ctx->revert_vts[self_ctx->r_app_indx%REVERT_WINDOW] = self_ctx->virtual_ts;
        self_ctx->r_app_indx++;
    }
  }

  // update version of lists and myself
  AS(lst, lst->head).version = lst->head_v;
  lst->head_v_arr[lst->head_v_idx].vts = AS_CTX(lst, lst->head).virtual_ts;
  lst->head_v_arr[lst->head_v_idx].version = lst->head_v++;
  lst->head_v_idx = (lst->head_v_idx + 1) % VERSION_WINDOW;
#endif 

  orig_sem_post(&lst->sem);
}

// Syncs all threads to context's virtual timestamp
void sync_t(task_list *lst, t_context *ctx) {
  // Reorder head in task_list
  reschedule(lst, ctx);
  assert(get_size(lst) > 0);
  yield(lst, ctx);
}

// not used
int is_pair(int updater_func_tag1, int updatee_func_tag2) {
    // return 1;
    if (updatee_func_tag2 == 0) return 1;
    if (updater_func_tag1 + updatee_func_tag2 == 0) return 1;
    if (updatee_func_tag2/100 == 0 ){
        int x = updatee_func_tag2, y = updater_func_tag1;
        if(x/100 > 0 && x%100 == 0 && (y == x/100 || -y == x/100)){
            return 1;
        }
    }
    return 0;
}


void vts_barrier(task_list *lst, task_list* out_law_lst, t_context *ctx) {
  DEBUG("Thread<%lu>: enter barrier %lu\n", (unsigned long)(ctx->tid), ctx->virtual_ts);
  sync_t(lst, self_ctx);

  // update outlaw vts;
  orig_sem_wait(&out_law_lst->sem);
  // Loop through the out_law_lst linked list
  int curr_idx = out_law_lst->head;
  while (curr_idx != NO_NEXT) {
    if (true && is_pair(self_ctx->func_tag, AS_CTX(out_law_lst, curr_idx).func_tag)){
        if (AS_CTX(out_law_lst, curr_idx).freeze_vts == 0) {
            // Update the virtual timestamp
            if (self_ctx->virtual_ts > AS_CTX(out_law_lst, curr_idx).virtual_ts) {
                assert(self_ctx->virtual_ts >= AS_CTX(out_law_lst, curr_idx).virtual_ts);
                AS_CTX(out_law_lst, curr_idx).virtual_ts = self_ctx->virtual_ts;
                AS_CTX(out_law_lst, curr_idx).vts_updated = 1;
                DEBUG_H("W: Update <Thread %lu> from %lu vts to %lu\n", (unsigned long)(AS_CTX(outlaw_lst, curr_idx).tid), AS_CTX(outlaw_lst, curr_idx).virtual_ts/1000000, self_ctx->virtual_ts/1000000);
            }
            
        }
    }
    curr_idx = AS(out_law_lst, curr_idx).next;
  }
  orig_sem_post(&out_law_lst->sem);
}

void mv_vts_to_next(task_list *lst) {

    orig_sem_wait(&lst->sem);
    int current_index = lst->head;
    while (current_index != NO_NEXT && AS_CTX(lst, current_index).tid != self_ctx->tid) {
        current_index = lst->tasks[current_index].next;
    }
    if (current_index != NO_NEXT) {
        // found my id
        int next_index = AS(lst, current_index).next;
        if (next_index == NO_NEXT) {
            orig_sem_post(&lst->sem);
            return;
        }
        uint64_t next_vts = AS_CTX(lst, next_index).virtual_ts;
        uint64_t vm_time = AS_CTX(lst, next_index).vm_time;
        self_ctx->virtual_ts = next_vts;
        self_ctx->vm_time = vm_time; 
        orig_sem_post(&lst->sem);
        sync_t(lst, self_ctx);
    }else{
        orig_sem_post(&lst->sem);
    }
}

void initialize_functions() {
  // read / write syscall
#if CONFIG_PERF
// internal to accvm
  perf_ins_func("get_real_ts");
  perf_ins_func("outlaw_updated");
  perf_ins_func("read");
  perf_ins_func("write");
  perf_ins_func("readv");
  perf_ins_func("writev");
  perf_ins_func("pread");
  perf_ins_func("pread64");
  perf_ins_func("pwrite");
  perf_ins_func("pwrite64");
  perf_ins_func("fork");
  perf_ins_func("vfork");
  perf_ins_func("exit_group");
  perf_ins_func("wait");
  perf_ins_func("waitpid");
  perf_ins_func("wait4");
  perf_ins_func("waitid");
  perf_ins_func("exit");
  perf_ins_func("sem_post");
  perf_ins_func("sem_wait");
  perf_ins_func("sem_timedwait");
  perf_ins_func("gettimeofday");
  perf_ins_func("clock_gettime");
  perf_ins_func("pthread_create");
  perf_ins_func("pthread_join");
  perf_ins_func("pthread_join_np");
  perf_ins_func("pthread_mutex_lock");
  perf_ins_func("pthread_mutex_unlock");
  perf_ins_func("pthread_mutex_trylock");
  perf_ins_func("pthread_cond_signal");
  perf_ins_func("pthread_broadcast");
  perf_ins_func("pthread_cond_wait");
  perf_ins_func("pthread_cond_timedwait");
  perf_ins_func("pthread_rwlock_rdlock");
  perf_ins_func("pthread_rwlock_wrlock");
  perf_ins_func("pthread_rwlock_unlock");
  perf_ins_func("pthread_rwlock_tryrdlock");
  perf_ins_func("pthread_rwlock_trywrlock");
  perf_ins_func("sched_yield");
  perf_ins_func("send");
  perf_ins_func("recv");
  perf_ins_func("listen");
  perf_ins_func("connect");
  perf_ins_func("accept");
  perf_ins_func("sendto");
  perf_ins_func("recvfrom");
  perf_ins_func("accept4");
  perf_ins_func("epoll_wait");
  perf_ins_func("select");
  perf_ins_func("poll");

  perf_ins_func("clock_nanosleep");
  perf_ins_func("nanosleep");

  perf_ins_func("fsync");

#endif
  orig_execve = (int (*)(const char *, char *const[], char *const[]))dlsym(RTLD_NEXT, "execve");
  orig_execle = (int (*)(const char *, const char *, ...))dlsym(RTLD_NEXT, "execle");
  orig_execl = (int (*)(const char *, const char *, ...))dlsym(RTLD_NEXT, "execl");
  orig_execlp = (int (*)(const char *, const char *, ...))dlsym(RTLD_NEXT, "execlp");
  orig_execv = (int (*)(const char *, char *const[]))dlsym(RTLD_NEXT, "execv");
  orig_execvp = (int (*)(const char *, char *const[]))dlsym(RTLD_NEXT, "execvp");
  orig_execvpe = (int (*)(const char *, char *const[], char *const[]))dlsym(RTLD_NEXT, "execvpe");
  orig_execveat = (int (*)(int, const char *, char *const[], char *const[], int))dlsym(RTLD_NEXT, "execveat");
  
  orig_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
  orig_write = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
  orig_readv = (ssize_t (*)(int, const struct iovec *, int))dlsym(RTLD_NEXT, "readv");
  orig_writev = (ssize_t (*)(int, const struct iovec *, int))dlsym(RTLD_NEXT, "writev");
  orig_pread = (ssize_t (*)(int, void *, size_t, off_t))dlsym(RTLD_NEXT, "pread");
  orig_pread64 = (ssize_t (*)(int, void *, size_t, off_t))dlsym(RTLD_NEXT, "pread64");
  orig_pwrite = (ssize_t (*)(int, const void *, size_t, off_t))dlsym(RTLD_NEXT, "pwrite");
  orig_pwrite64 = (ssize_t (*)(int, const void *, size_t, off_t))dlsym(RTLD_NEXT, "pwrite64");

  orig_fork = (pid_t (*)(void))dlsym(RTLD_NEXT, "fork");
  orig_vfork = (pid_t (*)(void))dlsym(RTLD_NEXT, "vfork");

  orig_wait = (pid_t (*)(int *))dlsym(RTLD_NEXT, "wait");
  orig_waitpid = (pid_t (*)(pid_t, int *, int))dlsym(RTLD_NEXT, "waitpid");
  orig_wait4 = (pid_t (*)(pid_t, int *, int, struct rusage *))dlsym(RTLD_NEXT, "wait4");
//   orig_waitid = (pid_t (*)(idtype_t, id_t, siginfo_t *, int))dlsym(RTLD_NEXT, "waitid");

  orig_sem_post = (int (*)(sem_t *))dlsym(RTLD_NEXT, "sem_post");
  orig_sem_wait = (int (*)(sem_t *))dlsym(RTLD_NEXT, "sem_wait");
  orig_sem_timedwait = (int (*)(sem_t *, const struct timespec *))dlsym(RTLD_NEXT, "sem_timedwait");

  orig_gettimeofday = (int (*)(struct timeval *, void *))dlsym(RTLD_NEXT, "gettimeofday");
  orig_clock_gettime = (int (*)(clockid_t, struct timespec *))dlsym(RTLD_NEXT, "clock_gettime");
  orig_clock_nanosleep = (int (*)(clockid_t, int, const struct timespec *, struct timespec *))dlsym(RTLD_NEXT, "clock_nanosleep");
  orig_nanosleep = (int (*)(const struct timespec *, struct timespec *))dlsym(RTLD_NEXT, "nanosleep");
  orig_pthread_create = (int (*)(pthread_t *, const pthread_attr_t *, void *(*start_routine)(void *),
                                 void *))dlsym(RTLD_NEXT, "pthread_create");

  orig_pthread_detach = (int (*)(pthread_t))dlsym(RTLD_NEXT, "pthread_detach");
  orig_clone = (int (*)(int (*)(void *), void *, int, void *, ...))dlsym(RTLD_NEXT, "clone");

  orig_pthread_join = (int (*)(pthread_t, void **))dlsym(RTLD_NEXT, "pthread_join");
  orig_pthread_join_np = (int (*)(pthread_t, void **))dlsym(RTLD_NEXT, "pthread_join_np");
  orig_pthread_mutex_lock = (int (*)(pthread_mutex_t *))dlsym(RTLD_NEXT, "pthread_mutex_lock");
  orig_pthread_mutex_unlock = (int (*)(pthread_mutex_t *))dlsym(RTLD_NEXT, "pthread_mutex_unlock");
  orig_pthread_mutex_trylock = (int (*)(pthread_mutex_t *))dlsym(RTLD_NEXT, "pthread_mutex_trylock");
  orig_pthread_cond_signal = (int (*)(pthread_cond_t *))dlsym(RTLD_NEXT, "pthread_cond_signal");
  orig_pthread_broadcast = (int (*)(pthread_cond_t *))dlsym(RTLD_NEXT, "pthread_broadcast");
  orig_pthread_cond_wait = (int (*)(pthread_cond_t *, pthread_mutex_t *))dlsym(RTLD_NEXT, "pthread_cond_wait");
  orig_pthread_cond_timedwait = (int (*)(pthread_cond_t *, pthread_mutex_t *, const struct timespec *))dlsym(RTLD_NEXT, "pthread_cond_timedwait");
  orig_pthread_rwlock_rdlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_rdlock");
  orig_pthread_rwlock_wrlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_wrlock");
  orig_pthread_rwlock_unlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_unlock");
  orig_pthread_rwlock_tryrdlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_tryrdlock");
  orig_pthread_rwlock_trywrlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_trywrlock");
  orig_sched_yield = (int (*)(void))dlsym(RTLD_NEXT, "sched_yield");

  orig_send = (ssize_t (*)(int, const void *, size_t, int ))dlsym(RTLD_NEXT, "send");
  orig_recv = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_NEXT, "recv");

  orig_listen = (int (*)(int, int))dlsym(RTLD_NEXT, "listen");
  orig_connect = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");

  orig_accept = (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
  orig_sendto = (ssize_t (*)(int, const void *, size_t, int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "sendto");
  orig_recvfrom = (ssize_t (*)(int, void *, size_t, int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "recvfrom");

  orig_accept4 = (int (*)(int, struct sockaddr *, socklen_t *, int))dlsym(RTLD_NEXT, "accept4");
  orig_epoll_wait = (int (*)(int, struct epoll_event *, int, int))dlsym(RTLD_NEXT, "epoll_wait");
  orig_select = (int (*)(int, fd_set *, fd_set *, fd_set *, struct timeval *))dlsym(RTLD_NEXT, "select");
  orig_poll = (int (*)(struct pollfd *, nfds_t, int))dlsym(RTLD_NEXT, "poll");

  orig_fsync = (int (*)(int fd))dlsym(RTLD_NEXT, "fsync");

}
