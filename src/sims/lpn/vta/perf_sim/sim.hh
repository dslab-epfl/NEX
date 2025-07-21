#include <assert.h>
#include <bits/stdint-uintn.h>
#include <chrono>
#include <clocale>
#include "lpn_funcs.hh"  
#include "places.hh"
#include "transitions.hh"
#include "lpn_init.hh"
#include "sims/lpn/lpn_common/place_transition.hh"
#include "sims/lpn/vta/include/driver.hh"
#include "transitions.hh"
#include "places.hh"

#define T_SIZE 21

Transition* t_list[T_SIZE] = { &dma_read_arbiter, &dma_read_mem_get, &dma_read_recv_from_mem, &dma_read_mem_put, &dma_write_arbiter, &dma_write_mem_get, &dma_write_recv_from_mem, &dma_write_mem_put, &t13, &tload_insn_pre, &tload_insn_post, &t12, &t14, &t15, &t16, &tload_launch, &load_done, &store_launch, &store_done, &compute_launch, &compute_done };;

void lpn_start(uint64_t insn_count, uint64_t ps_ts){

    NEW_TOKEN(token_class_total_insn, plaunch_tk);
    plaunch_tk->total_insn = insn_count;
    plaunch_tk->ts = ps_ts;
    plaunch.pushToken(plaunch_tk);
    // pload_done.reset();
    // pstore_done.reset();
    // pcompute_done.reset();

    printf("setup_LPN done\n");
}

int lpn_finished(){
    if(!(dma_read_requests.empty() && dma_write_requests.empty() && dma_read_resp.empty() && dma_write_resp.empty())){
        return 0;
    }
    uint64_t next_ts = NextCommitTime(t_list, T_SIZE);
    if(next_ts == lpn::LARGE){
        return 1;
    }
    return 0;
}

int lpn_clear(){
    
    int load2compute = pload2compute.tokensLen();
    int compute2store = pcompute2store.tokensLen();
    int store2compute = pstore2compute.tokensLen();
    int compute2load = pcompute2load.tokensLen();

    for(int i=0; i<T_SIZE; i++){
        for(auto p : t_list[i]->p_input){
            p->reset();
        }
        for (auto p : t_list[i]->p_output){
            p->reset();
        }
    }

    lpn_init();

    for(int i=0; i < load2compute; i++){
        NEW_TOKEN(EmptyToken, new_token);
        pload2compute.pushToken(new_token);
    }

    for(int i=0; i < compute2store; i++){
        NEW_TOKEN(EmptyToken, new_token);
        pcompute2store.pushToken(new_token);
    }

    for(int i=0; i < store2compute; i++){
        NEW_TOKEN(EmptyToken, new_token);
        pstore2compute.pushToken(new_token);
    }

    for(int i=0; i < compute2load; i++){
        NEW_TOKEN(EmptyToken, new_token);
        pcompute2load.pushToken(new_token);
    }

    return 0;
}