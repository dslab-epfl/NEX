#include <assert.h>
#include <bits/stdint-uintn.h>
#include <chrono>
#include "place_transition.hh"
#include "lpn_funcs.hh"  
#include "places.hh"
#include "transitions.hh"
#include "lpn_init.hh"
#include "../include/driver.hh"

static uint64_t total_tasks = 0;

#define T_SIZE 139
#define LOOP_TS(func) for(int i=0;i < T_SIZE; i++){ \
        Transition* t = protoacc_t_list[i]; \
        func; \
    }  

Transition* protoacc_t_list[T_SIZE] = { &t1, &t2, &t19, &tinject_write_dist, &t24, &tdispatch_dist, &twrite_dist, &t3_pre, &t3_post, &t10, &t9_pre, &t9_post, &split_msg, &tload_field_addr, &tload_field_addr_post, &t23_pre, &t23_post, &f1_resume, &f1_dist, &f1_eom, &f1_25, &f1_26, &f1_28, &f1_31, &f1_40, &f1_30_pre, &f1_30_post, &f1_36_pre, &f1_36_post, &f1_44_pre, &f1_44_post, &f1_37_pre, &f1_37_post, &f1_37_post2, &f1_dispatch, &f1_write_req_out, &f2_resume, &f2_dist, &f2_eom, &f2_25, &f2_26, &f2_28, &f2_31, &f2_40, &f2_30_pre, &f2_30_post, &f2_36_pre, &f2_36_post, &f2_44_pre, &f2_44_post, &f2_37_pre, &f2_37_post, &f2_37_post2, &f2_dispatch, &f2_write_req_out, &f3_resume, &f3_dist, &f3_eom, &f3_25, &f3_26, &f3_28, &f3_31, &f3_40, &f3_30_pre, &f3_30_post, &f3_36_pre, &f3_36_post, &f3_44_pre, &f3_44_post, &f3_37_pre, &f3_37_post, &f3_37_post2, &f3_dispatch, &f3_write_req_out, &f4_resume, &f4_dist, &f4_eom, &f4_25, &f4_26, &f4_28, &f4_31, &f4_40, &f4_30_pre, &f4_30_post, &f4_36_pre, &f4_36_post, &f4_44_pre, &f4_44_post, &f4_37_pre, &f4_37_post, &f4_37_post2, &f4_dispatch, &f4_write_req_out, &f5_resume, &f5_dist, &f5_eom, &f5_25, &f5_26, &f5_28, &f5_31, &f5_40, &f5_30_pre, &f5_30_post, &f5_36_pre, &f5_36_post, &f5_44_pre, &f5_44_post, &f5_37_pre, &f5_37_post, &f5_37_post2, &f5_dispatch, &f5_write_req_out, &f6_resume, &f6_dist, &f6_eom, &f6_25, &f6_26, &f6_28, &f6_31, &f6_40, &f6_30_pre, &f6_30_post, &f6_36_pre, &f6_36_post, &f6_44_pre, &f6_44_post, &f6_37_pre, &f6_37_post, &f6_37_post2, &f6_dispatch, &f6_write_req_out, &dma_read_port_arbiter, &dma_read_port_mem_get, &dma_read_port_recv_from_mem, &dma_read_port_mem_put, &dma_write_port_arbiter, &dma_write_port_mem_get, &dma_write_port_recv_from_mem, &dma_write_port_mem_put };;

static uint64_t in_flight_read_write = 0;

void advance_mem(uint64_t ts){
    // mem_advance_until_time(ts);
    // mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr
    while(!dma_read_requests.empty()){
        auto token = dma_read_requests.front();
        dma_read_requests.pop_front();
        // printf("mem_put: tag %d, port %d\n", token->id, token->ref);
        token->ts = ts;
        dma_read_resp.push_back(token);
    }

    while(!dma_write_requests.empty()){
        auto token = dma_write_requests.front();
        dma_write_requests.pop_front();
        token->ts = ts;
        // printf("mem_put: tag %d, port %d\n", token->id, token->ref);
        dma_write_resp.push_back(token);
    }

}

void __advance_mem(uint64_t ts){
    mem_advance_until_time(ts);
    // mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr
    while(!dma_read_requests.empty()){
        token_class_iasbrr* token = dma_read_requests.front();
        dma_read_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        int matched = 0;
        if(io_req_map[tag].empty()){
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            dma_read_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        assert(req != nullptr);
        req->extra_ptr = static_cast<void*>(token);
        assert(req->tag == tag);
        // printf("@%ld[ns] dma_read_start id %d; port %d tag %d\n", ts/1000, req->id, token->ref, token->id);
        in_flight_read_write++;
        mem_put(ts, req->id, req->addr, req->len, 0, 0, req->rw, req->tag);
        io_pending_map[tag].push_back(std::move(req));
    }

    while(!dma_write_requests.empty()){
        auto token = dma_write_requests.front();
        dma_write_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        int matched = 0;
        if(io_req_map[tag].empty()){
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            dma_write_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        req->extra_ptr = static_cast<void*>(token);
        // printf("mem_put: tag %d, id %d, port %d\n", tag, req->id, token->ref);
        // printf("@%ld[ns] dma_write_start id %d; port %d tag %d\n", ts/1000, req->id, token->ref, token->id);
        in_flight_read_write++;
        mem_put(ts, req->id, req->addr, req->len, 0, 0, req->rw, req->tag);
        io_pending_map[tag].push_back(std::move(req));
    }

    // int mem_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len)
    uint64_t tag, id, buffer, len;
    while(mem_get(&tag, &id, &buffer, &len)){
        in_flight_read_write--;
        // printf("mem_get: tag %lu, id %lu, buffer %lu, len %lu\n", tag, id, buffer, len);
        auto it = io_pending_map[tag].begin();
        while(it != io_pending_map[tag].end()){
            auto req = it->get();
            if(req->id == id){
                if(req->rw == 0){
                    // dma_read_resp
                    auto tk = (token_class_iasbrr*)req->extra_ptr;
                    tk->ts = ts;
                    tk->size = len;
                    // printf("@%ld[ns] dma_read_done id %d; port %d tag %d\n", ts/1000, req->id, tk->ref, tk->id);
                    dma_read_resp.push_back(tk);
                }else{
                    auto tk = (token_class_iasbrr*)req->extra_ptr;
                    tk->ts = ts;
                    tk->size = len;
                    // printf("@%ld[ns] dma_read_done id %d; port %d tag %d\n", ts/1000, req->id, tk->ref, tk->id);
                    dma_write_resp.push_back(tk);
                }   
                io_pending_map[tag].erase(it);
                break;
            }
            it++;
        }
    }
}

// this is not necessary to implement for every accelerators
void protoacc_advance_until_time(uint64_t ts_in_nano){
   
    uint64_t ts = ts_in_nano*1000;
    uint64_t time = 0, prev_time = 0; 
    uint64_t start = 0;
    while(time <= ts){
        while(time <= ts){
            time = trigger_and_min_time_g(protoacc_t_list, T_SIZE);
            // LOOP_TS(trigger(t));
            // time = min_time_g(protoacc_t_list, T_SIZE);
            if(time == lpn::LARGE || time > ts){
                break;
            }
            if(start == 0){
                start = time;
            }
            prev_time = time;
            LOOP_TS(sync(t, time));
            advance_mem(time);
        }
        if(time == lpn::LARGE){
            uint64_t nx_active_time = 0;
            int active = mem_active(&nx_active_time);
            // printf("mem active: %d, time %ld\n", active, nx_active_time);
            if(active == 0){
                break;
            }
            // the protoacc lpn has no events, but mem might have events
            advance_mem(nx_active_time);
            time = nx_active_time;
        }else{
            break;
        }
    }
    // for (int i = 0; i < T_SIZE; i++){
    //     auto t = protoacc_t_list[i];
    //     printf("t %s, t->count %d\n", t->id.c_str(), t->count);
    // }
    
    // for (int i = 0; i < T_SIZE; i++){
    //     auto t = protoacc_t_list[i];
    //     // loop through input places and output places
    //     for (int j = 0; j < t->p_input.size(); j++){
    //         auto p = t->p_input[j];
    //         if(p->tokensLen() > 0){
    //             printf("t %s, p %s, p->tokens.size() %d\n", t->id.c_str(), p->id.c_str(), p->tokensLen());
    //         }
    //     }
    //     for (int j = 0; j < t->p_output.size(); j++){
    //         auto p = t->p_output[j];
    //         if(p->tokensLen() > 0){
    //             printf("t %s, p %s, p->tokens.size() %d\n", t->id.c_str(), p->id.c_str(), p->tokensLen());
    //         }
    //     }
    // }
    // printf(" //// (end %lu, start%lu)lpn advanced this much: %lu[ns]\n", prev_time/1000, start/1000, (prev_time - start)/1000);
}

// here it's simplified to check if all transitions are finished
// this is wrong if there are multiple accelerators connected
int protoacc_perf_sim_finished(uint64_t* finished_task){
    // no outstanding requests or responses
   
    if(in_flight_read_write > 0){
        return 0;
    }

    if(!(dma_read_requests.empty() && dma_write_requests.empty() && dma_read_resp.empty() && dma_write_resp.empty())){
        return 0;
    }

    uint64_t time = trigger_and_min_time_g(protoacc_t_list, T_SIZE);
    if (time == lpn::LARGE){
        printf("lpn finished \n");
        // for(int tag=0; tag<8; tag++){
        //     printf("total req in tag %d: %lu\n", tag, io_req_map[tag].size());
        // }
        
        for (int i = 0; i < T_SIZE; i++){
            auto t = protoacc_t_list[i];
            printf("t %s, t->count %d\n", t->id.c_str(), t->count);
        }
        for(int tag=0; tag<8; tag++){
            io_req_map[tag].clear();
        }
        *finished_task = total_tasks;
        return 1;
    }
    return 0;
}

extern "C"{
    extern void protoacc_func_sim(int, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t ts);
}

void protoacc_sim_start(int pid, uint64_t msg_start_addr, uint64_t serialized_msg_addr, uint64_t descriptor_table_addr, uint64_t src_base_addr, uint64_t ns_ts){
    total_tasks ++;
    protoacc_func_sim(pid, msg_start_addr, serialized_msg_addr, descriptor_table_addr, src_base_addr, ns_ts*1000);
    // setup_LPN(ns_ts*1000);
    for(int tag=0; tag<8; tag++){
        printf("total req in tag %d: %lu\n", tag, io_req_map[tag].size());
    }
    printf("setup_LPN done\n");
}