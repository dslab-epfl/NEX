#include <assert.h>
#include <bits/stdint-uintn.h>
#include "place_transition.hh"
#include "lpn_funcs.hh"  
#include "places.hh"
#include "transitions.hh"
//#include "setup.hh"
#include "lpn_init.hh"
#include "../func_sim/mem_ops.hh"
#include "cache.hh"
#include <pthread.h>
#include <config/config.h>

// #define NEX_SPIN_LOCK
#ifdef NEX_SPIN_LOCK
SpinLock lock;
#define LOCK lock.lock();
#define UNLOCK lock.unlock();
#else
static pthread_mutex_t mem_lock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK pthread_mutex_lock(&mem_lock)
#define UNLOCK pthread_mutex_unlock(&mem_lock)
#endif

#define T_SIZE 4
#define LOOP_TS(func) for(int i=0;i < T_SIZE; i++){ \
        Transition* t = mem_t_list[i]; \
        func; \
    }  


Transition* mem_t_list[T_SIZE] = {&tprocess_mem_req_front, &tlatency_sim, &tforge_mem_resp, &distributed_mem_resp};

Cache cache_sim(CONFIG_CACHE_SIZE*1024, CONFIG_CACHE_ASSOC, 64, CONFIG_CACHE_HIT_LATENCY, CONFIG_CACHE_MISS_FETCH_LATENCY, CONFIG_CACHE_FIRST_HIT);
// this is for old vta
// Cache cache_sim(30*1048576, 11, 64, 20, 72, false);

// this is for new vta and protoacc and jpeg
// Cache cache_sim(30*1048576, 11, 64, 20, 62, false);

//move the cache to L2
// Cache cache_sim(1*1048576, 11, 64, 4, 20, false);

// purely dram
// Cache cache_sim(1048576, 11, 64, 60, 0, false);

//Cache cache_sim(30*1048576, 11, 64, 17, 59, false);

// Cache cache_sim(30*1048576, 11, 64, 16, 40, false);

// this is not necessary to implement for every accelerators
void mem_advance_until_time(uint64_t ts){
    LOCK;
    uint64_t time = 0, start_time = 0; 
    while(time <= ts){
        time = trigger_and_min_time_g(mem_t_list, T_SIZE);
        // safe_printf("lpn next time: %lu\n", time);
        if(time == lpn::LARGE || time > ts){
           break;
        }
        if(start_time == 0){
            start_time = time;
        }
        LOOP_TS(sync(t, time));
    }
    UNLOCK;
    // safe_printf("Mem lpn advanced by: %lu, to %lu \n", last_time-start_time, ts);
}

int mem_active(uint64_t* next_active_ts){
    LOCK;
    uint64_t time = trigger_and_min_time_g(mem_t_list, T_SIZE);
    if(time == lpn::LARGE){
        UNLOCK;
        return 0;
    }
    *next_active_ts = time;
    UNLOCK;
    return 1;
}

// here it's simplified to check if all transitions are finished
// this is wrong if there are multiple accelerators connected
// put a memory request
int mem_put(uint64_t dev_id, uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr){
    // create a new token
    LOCK;
    NEW_TOKEN(token_class_ralmtrd, token);
    token->dev_id = dev_id;
    token->req_id = req_id;
    token->addr = addr;
    token->len = len;
    token->mem_buffer_ptr = mem_buffer_ptr;
    token->dma_fd = dma_fd;
    token->type = type;
    token->ref_ptr = ref_ptr;
    token->ts = virtual_ts;
    memory_req_front.pushToken(token);
    UNLOCK;
    return 0;
}

// get a memory request
int mem_get(uint64_t dev_id, uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len, uint64_t* ts){
    LOCK;
    auto memory_resp_for_this_dev = list_10_devices[dev_id];
    // safe_printf("mem_get: %p; dev_id %lu\n", memory_resp_for_this_dev, dev_id);
    if(memory_resp_for_this_dev->tokens.size() == 0){
        UNLOCK;
        return 0;
    }else{
        auto token = memory_resp_for_this_dev->tokens[0];
        *ref_ptr = (token->ref_ptr);
        *req_id = (token->req_id);
        *ts = (token->ts);
        
        if(buffer == NULL){
            memory_resp_for_this_dev->popToken();
            if(token->type == 1){
                *len = 0;
            }else{
                *len = (token->len);
            }
            UNLOCK;
            return 1;
        }

        if(token->type == 1){
            *buffer = 0;
            *len = 0;
            mem_write(token->dma_fd, token->addr, (void*)token->mem_buffer_ptr, (size_t)token->len);
        }else{
            mem_read(token->dma_fd, token->addr, (void*)token->mem_buffer_ptr, (size_t)token->len);
            *buffer = (token->mem_buffer_ptr);
            *len = (token->len);
        }
        memory_resp_for_this_dev->popToken();
        UNLOCK;
        return 1;
    }
}

void mem_stats(){
    safe_printf("Cache average latency: %d\n", cache_sim.average_latency());
    safe_printf("Cache hit rate: %f\n", cache_sim.hit_rate());
}
