#include <assert.h>
#include <bits/stdint-uintn.h>
#include <chrono>
#include "place_transition.hh"
#include "lpn_funcs.hh"  
#include "places.hh"
#include "transitions.hh"
#include "lpn_init.hh"
#include "../include/driver.hh"
#include "../include/vta/driver.h"
#include <config/config.h>

extern "C" {
#if CONFIG_PCIE_LPN
    void pcie_advance_until_time(uint64_t ts);
    int pcie_active(uint64_t* next_active_ts);
    int pcie_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr);
    int pcie_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len);
#else
    void mem_advance_until_time(uint64_t ts);
    int mem_active(uint64_t* next_active_ts);
    int mem_put(int dev_id, uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr);
    int mem_get(int dev_id, uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len, uint64_t* ts);
    // int mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr);
    // int mem_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len);
#endif
}

#if CONFIG_PCIE_LPN
#define DMA_ADVANCE_UNTIL_TIME(ts) pcie_advance_until_time(ts)
#define DMA_ACTIVE(next_active_ts) pcie_active(next_active_ts)
#define DMA_PUT(...) pcie_put(__VA_ARGS__)
#define DMA_GET(...) pcie_get(__VA_ARGS__)
#else
#define DMA_ADVANCE_UNTIL_TIME(ts) mem_advance_until_time(ts)
#define DMA_ACTIVE(next_active_ts) mem_active(next_active_ts)
#define DMA_PUT(...) mem_put(__VA_ARGS__)
#define DMA_GET(...) mem_get(__VA_ARGS__)
#endif


#define T_SIZE 21
#define LOOP_TS(func) for(int i=0;i < T_SIZE; i++){ \
        Transition* t = vta_t_list[i]; \
        func; \
    }  

#define DEV_ID 0

static uint64_t inflight_read_write = 0;

Transition* vta_t_list[T_SIZE] = {&vta::dma_read_arbiter, &vta::dma_read_mem_get, &vta::dma_read_recv_from_mem, &vta::dma_read_mem_put, &vta::dma_write_arbiter, &vta::dma_write_mem_get, &vta::dma_write_recv_from_mem, &vta::dma_write_mem_put, &vta::t13, &vta::tload_insn_pre, &vta::tload_insn_post, &vta::t12, &vta::t14, &vta::t15, &vta::t16, &vta::tload_launch, &vta::load_done, &vta::store_launch, &vta::store_done, &vta::compute_launch, &vta::compute_done };;


void advance_dma(uint64_t ts){
    DMA_ADVANCE_UNTIL_TIME(ts);

    // int mem_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len)
    uint64_t tag, id, buffer, len, the_vts;
    while(DMA_GET(DEV_ID, &tag, &id, &buffer, &len, &the_vts)){
        inflight_read_write --;
        // printf("mem_get: tag %lu, id %lu, buffer %lu, len %lu\n", tag, id, buffer, len);
        auto it = io_pending_map[tag].begin();
        while(it != io_pending_map[tag].end()){
            auto req = it->get();
            if(req->id == id){
                if(req->rw == 0){
                    // dma_read_resp
                    auto tk = (token_class_iasbrr*)req->extra_ptr;
                    tk->ts = ts;
                    // printf("@%ld[ns] dma_read_done id %d; port %d tag %d, len %d\n", ts/1000, req->id, tk->ref, tk->id, tk->size);
                    vta::dma_read_resp.push_back(tk);
                }else{
                    auto tk = (token_class_iasbrr*)req->extra_ptr;
                    tk->ts = ts;
                    // printf("@%ld[ns] dma_read_done id %d; port %d tag %d\n", ts/1000, req->id, tk->ref, tk->id);
                    vta::dma_write_resp.push_back(tk);
                }
                io_pending_map[tag].erase(it);
                break;
            }
            it++;
        }
    }

    // mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr
    while(!vta::dma_read_requests.empty()){
        token_class_iasbrr* token = vta::dma_read_requests.front();
        vta::dma_read_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        int matched = 0;
        if(io_req_map[tag].empty()){
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            vta::dma_read_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        assert(req != nullptr);
        req->extra_ptr = static_cast<void*>(token);
        assert(req->tag == tag);
        token->size = req->len;
        // printf("@%ld[ns] dma_read_start id %d; len %d\n", ts/1000, req->id, req->len);
        DMA_PUT(DEV_ID, ts, req->id, req->addr, req->len, 0, 0, req->rw, req->tag);
        inflight_read_write ++;
        io_pending_map[tag].push_back(std::move(req));
    }

    while(!vta::dma_write_requests.empty()){
        auto token = vta::dma_write_requests.front();
        vta::dma_write_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        int matched = 0;
        if(io_req_map[tag].empty()){
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            vta::dma_write_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        req->extra_ptr = static_cast<void*>(token);
        token->size = req->len;
        // printf("mem_put: tag %d, id %d, port %d\n", tag, req->id, token->ref);
        // printf("@%ld[ns] dma_write_start id %d; len %d\n", ts/1000, req->id, req->len);
        DMA_PUT(DEV_ID, ts, req->id, req->addr, req->len, 0, 0, req->rw, req->tag);
        inflight_read_write ++;
        io_pending_map[tag].push_back(std::move(req));
    }

}

// this is not necessary to implement for every accelerators
void vta_advance_until_time(uint64_t ts_in_nano){
    
    uint64_t ts = ts_in_nano*1000;
    uint64_t time = 0, prev_time = 0; 
    uint64_t start = 0;
    while(time <= ts){
        while(time <= ts){
            time = trigger_and_min_time_g(vta_t_list, T_SIZE);
            uint64_t child_active_time = 0;
            int active = DMA_ACTIVE(&child_active_time);
            if (active == 1 && time > child_active_time){
                time = child_active_time;
            }
            if(time == lpn::LARGE || time > ts){
                break;
            }
            if(start == 0){
                start = time;
            }
            prev_time = time;
            for(int i=0;i < T_SIZE; i++){
                Transition* t = vta_t_list[i];
                int fired = sync(t, time);
            }
            advance_dma(time);
        }
        if(time == lpn::LARGE){
            uint64_t nx_active_time = 0;
            int active = DMA_ACTIVE(&nx_active_time);
            // printf("mem active: %d, time %ld\n", active, nx_active_time);
            if(active == 0){
                break;
            }
            // the vta lpn has no events, but mem might have events
            advance_dma(nx_active_time);
            time = nx_active_time;
        }else{
            break;
        }
    }
   
}

// here it's simplified to check if all transitions are finished
// this is wrong if there are multiple accelerators connected
int vta_perf_sim_finished(){
    // no outstanding requests or responses
    if(inflight_read_write > 0){
        return 0;
    }

    if(!(vta::dma_read_requests.empty() && vta::dma_write_requests.empty() && vta::dma_read_resp.empty() && vta::dma_write_resp.empty())){
        return 0;
    }

    uint64_t child_active_time = 0;
    if (DMA_ACTIVE(&child_active_time) == 1){
        return 0;
    }

    uint64_t time = trigger_and_min_time_g(vta_t_list, T_SIZE);
    if (time == lpn::LARGE){
    
        // for(auto &kv : io_req_map) {
        //     std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
        // }
        // printf("dma_read_resp size %lu, dma_write_resp size %lu\n", vta::dma_read_resp.size(), vta::dma_write_resp.size());
        for(auto &kv : io_req_map) {
            // if(kv.first == 0){
            //     for (int i = 0; i < T_SIZE; i++){
            //         auto t = vta_t_list[i];
            //         // loop through input places and output places
            //         for (int j = 0; j < t->p_input.size(); j++){
            //             auto p = t->p_input[j];
            //             if(p->tokensLen() > 0){
            //                 printf("t %s, p %s, p->tokens.size() %d\n", t->id.c_str(), p->id.c_str(), p->tokensLen());
            //             }
            //         }
            //         for (int j = 0; j < t->p_output.size(); j++){
            //             auto p = t->p_output[j];
            //             if(p->tokensLen() > 0){
            //                 printf("t %s, p %s, p->tokens.size() %d\n", t->id.c_str(), p->id.c_str(), p->tokensLen());
            //             }
            //         }
            //     }
            // }
            assert(kv.second.size() == 0);
        }
        return 1;
    }
    return 0;
}

void vta_sim_start(uint64_t insn_phy_addr,
                 uint64_t dma_base_addr,
                 uint32_t insn_count,
                 uint32_t wait_cycles,
                 uint64_t ts_in_nano){

    inflight_read_write = 0;
    uint64_t ts = ts_in_nano*1000; // in picoseconds
    // printf("vta_sim_start\n");
    VTADeviceHandle vta_func_device = AccVMVTADeviceAlloc(dma_base_addr);
    // printf("vta_func_device: %p, insn_count %d\n", vta_func_device, insn_count);
    AccVMVTADeviceRun(vta_func_device, insn_phy_addr, insn_count, wait_cycles, ts);
    // printf("after run: %p\n", vta_func_device);
    NEW_TOKEN(token_class_total_insn, plaunch_tk);
    plaunch_tk->total_insn = insn_count;
    plaunch_tk->ts = ts;
    vta::plaunch.pushToken(plaunch_tk);
   
    // for(auto &kv : io_req_map) {
    //     std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
    // }

    // if(io_req_map[0].size() == 448 && io_req_map[1].size() == 1792 && io_req_map[2].size() == 25088 && io_req_map[3].size() == 28){
    //     std::set<std::string> place_set;
    //     for(int i = 0; i < T_SIZE; i++){
    //         auto t = vta_t_list[i];
    //         for (auto p : t->p_output){
    //             if(place_set.find(p->id) == place_set.end()){
    //                 place_set.insert(p->id);
    //                 std::cerr << "Place:"<< p->id << " token count=" << p->tokensLen() << "\n";
    //             }
    //         }

    //         for (auto p : t->p_input){
    //             if(place_set.find(p->id) == place_set.end()){
    //                 place_set.insert(p->id);
    //                 std::cerr << "Place:"<< p->id << " token count=" << p->tokensLen() << "\n";
    //             }
    //         }
    //     }
    //     exit(1);
    // }
}