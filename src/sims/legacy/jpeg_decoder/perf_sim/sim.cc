#include <assert.h>
#include <bits/stdint-uintn.h>
#include <sys/types.h>
#include <chrono>
#include "place_transition.hh"
#include "lpn_funcs.hh"  
#include "lpn_init.hh"
#include "../include/driver.hh"
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"
#include "transitions.hh"
#include "places.hh"
#include <config/config.h>

extern "C" {
#if CONFIG_PCIE_LPN
    void pcie_advance_until_time(uint64_t ts);
    int pcie_active(uint64_t* next_active_ts);
    int pcie_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr);
    int pcie_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len);
#else
    void mem_advance_until_time(uint64_t ts);
    int mem_active(uint64_t* next_active_ts);
    int mem_put(int dev_id, uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr);
    int mem_get(int dev_id, uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len, uint64_t* ts);
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

#define T_SIZE 20
#define LOOP_TS(func) for(int i=0;i < T_SIZE; i++){ \
        Transition* t = jpeg_t_list[i]; \
        func; \
    }

#define DEV_ID 0

static uint64_t inflight_read_write = 0;

Transition* jpeg_t_list[T_SIZE] =  {&jpeg::dma_read_arbiter, &jpeg::dma_read_mem_get, &jpeg::dma_read_recv_from_mem, &jpeg::dma_read_mem_put, &jpeg::dma_write_arbiter, &jpeg::dma_write_mem_get, &jpeg::dma_write_recv_from_mem, &jpeg::dma_write_mem_put, &jpeg::twait_header, &jpeg::t2, &jpeg::t3, &jpeg::t4, &jpeg::t5, &jpeg::t0, &jpeg::t1, &jpeg::twait_header, &jpeg::tinp_fifo, &jpeg::toutput_fifo, &jpeg::t_input_fifo_recv, &jpeg::toutput_fifo_recv };

void advance_dma(uint64_t ts){
    DMA_ADVANCE_UNTIL_TIME(ts);
    // mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr
    while(!jpeg::dma_read_requests.empty()){
        jpeg::token_class_iasbrr* token = jpeg::dma_read_requests.front();
        jpeg::dma_read_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        int matched = 0;
        if(io_req_map[tag].empty()){
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            jpeg::dma_read_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        assert(req != nullptr);
        req->extra_ptr = static_cast<void*>(token);
        assert(req->tag == tag);
        // printf("mem_put: tag %d, id %d, port %d, len %u\n", tag, req->id, token->ref, req->len);
        // printf("@%ld[ns] dma_read_start id %d; port %d tag %d\n", ts/1000, req->id, token->ref, token->id);
        DMA_PUT(DEV_ID, ts, req->id, req->addr, req->len, 0, 0, req->rw, req->tag);
        inflight_read_write ++;
        io_pending_map[tag].push_back(std::move(req));
    }

    while(!jpeg::dma_write_requests.empty()){
        auto token = jpeg::dma_write_requests.front();
        jpeg::dma_write_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        int matched = 0;
        if(io_req_map[tag].empty()){
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            jpeg::dma_write_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        req->extra_ptr = static_cast<void*>(token);
        // printf("mem_put: tag %d, id %d, port %d, len %u\n", tag, req->id, token->ref, req->len);
        // printf("@%ld[ns] dma_write_start id %d; port %d tag %d\n", ts/1000, req->id, token->ref, token->id);
        DMA_PUT(DEV_ID, ts, req->id, req->addr, req->len, 0, 0, req->rw, req->tag);
        inflight_read_write ++;
        io_pending_map[tag].push_back(std::move(req));
    }

    // int mem_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len)
    uint64_t tag, id, buffer, len, the_vts;
    while(DMA_GET(DEV_ID, &tag, &id, NULL, &len, &the_vts)){
        inflight_read_write --;
        // printf("mem_get: tag %lu, id %lu, buffer %lu, len %lu\n", tag, id, buffer, len);
        auto it = io_pending_map[tag].begin();
        while(it != io_pending_map[tag].end()){
            auto req = it->get();
            if(req->id == id){
                if(req->rw == 0){
                    // jpeg::dma_read_resp
                    auto tk = (jpeg::token_class_iasbrr*)req->extra_ptr;
                    tk->ts = ts;
                    // printf("@%ld[ns] dma_read_done id %d; port %d tag %d\n", ts/1000, req->id, tk->ref, tk->id);
                    jpeg::dma_read_resp.push_back(tk);
                }else{
                    auto tk = (jpeg::token_class_iasbrr*)req->extra_ptr;
                    tk->ts = ts;
                    // printf("@%ld[ns] dma_read_done id %d; port %d tag %d\n", ts/1000, req->id, tk->ref, tk->id);
                    jpeg::dma_write_resp.push_back(tk);
                }
                io_pending_map[tag].erase(it);
                break;
            }
            it++;
        }
    }
}

// here it's simplified to check if all transitions are finished
// this is wrong if there are multiple accelerators connected
int jpeg_perf_sim_finished(){
    // no outstanding requests or responses
    if(inflight_read_write > 0){
        return 0;
    }

    if(!(jpeg::dma_read_requests.empty() && jpeg::dma_write_requests.empty() && jpeg::dma_read_resp.empty() && jpeg::dma_write_resp.empty())){
        return 0;
    }

    uint64_t child_active_time = 0;
    if (DMA_ACTIVE(&child_active_time) == 1){
        return 0;
    }

    LOOP_TS(trigger(t));
    uint64_t time = min_time_g(jpeg_t_list, T_SIZE);
    if (time == lpn::LARGE){
        DMA_ACTIVE(&time);
        if (time != lpn::LARGE){
            return 0;
        }
        for(auto &kv : io_req_map) {
            std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
        }
        for(auto &kv : io_req_map) {
            // if(kv.first == 0){
            //     for (int i = 0; i < T_SIZE; i++){
            //         auto t = jpeg_t_list[i];
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
    // if (time == lpn::LARGE){
    //     printf("lpn finished \n");
    //     return 1;
    // }
    return 0;
}

// this is not necessary to implement for every accelerators
void jpeg_advance_until_time(uint64_t ts_in_nano){
    // for(int tag=0; tag<6; tag++){
    //     printf("total req in tag %d: %lu\n", tag, io_req_map[tag].size());
    // }

    uint64_t ts = ts_in_nano*1000;
    uint64_t time = 0, prev_time = 0; 
    uint64_t start = 0;
    while(time <= ts){
        while(time <= ts){
            LOOP_TS(trigger(t));
            time = min_time_g(jpeg_t_list, T_SIZE);
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
            LOOP_TS(sync(t, time));
            advance_dma(time);
        }
        if(time == lpn::LARGE){
            uint64_t nx_active_time = 0;
            int active = DMA_ACTIVE(&nx_active_time);
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
    // for (int i = 0; i < T_SIZE; i++){
    //     auto t = jpeg_t_list[i];
    //     printf("t %s, t->count %d\n", t->id.c_str(), t->count);
    // }
    
    // for (int i = 0; i < T_SIZE; i++){
    //     auto t = jpeg_t_list[i];
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


extern "C" {
    extern void jpeg_lpn_driver(uint8_t* buf, uint32_t len, uint8_t* dst, uint64_t* decoded_len, int** ret_array_ptr, int* size);
}


void jpeg_sim_start(uint8_t* buf, uint32_t len, uint8_t* dst, uint64_t decoded_len, uint64_t ts){
    
    uint64_t ps_ts = ts*1000;
    int* non_zeros;
    int size;
    int size_of_img = len;
    uint64_t img_addr = (uint64_t)buf;
    inflight_read_write = 0;
   
    jpeg_lpn_driver(buf, len, dst, &decoded_len, &non_zeros, &size);
    // this runs func-sim to completion and returns the features
    printf("func_sim: should have %d blocks, current ps_ts %ld\n", size*4/6, ps_ts);
   
    for (int i = 0; i < size; ++i){
        NEW_TOKEN(jpeg::token_class_nonzero, pvarlatency_0_tk);
        pvarlatency_0_tk->nonzero = non_zeros[i];
        pvarlatency_0_tk->ts = ps_ts;
        jpeg::pvarlatency.pushToken(pvarlatency_0_tk);
    }

    int size_of_data = size_of_img - 18*32;
    int avg_block_size = size_of_data/size;

    printf("total blocks : %d; and average block size %d\n", size, avg_block_size);
    
    jpeg::pvar_recv_token.reset();
    NEW_TOKEN(jpeg::token_class_avg_block_size, pvar_recv_token_0_tk);
    pvar_recv_token_0_tk->avg_block_size = avg_block_size;
    jpeg::pvar_recv_token.pushToken(pvar_recv_token_0_tk);

    uint64_t id_counter = 0;
    // for header read
    for(int i = 0; i < 18; i++){
        NEW_TOKEN(jpeg::token_class_req_len, p_req_mem_0_tk);
        p_req_mem_0_tk->req_len = 32;
        p_req_mem_0_tk->ts = ps_ts;
        jpeg::p_req_mem.pushToken(p_req_mem_0_tk);
        enqueueReq(id_counter++, img_addr+i*32, 32, (int)jpeg::CstStr::DMA_READ, 0);
    }
    // for header read end
    img_addr += 18*32;

    // for data read
    for(int i = 0; i < avg_block_size*size/32; i++){
        NEW_TOKEN(jpeg::token_class_req_len, p_req_mem_0_tk);
        p_req_mem_0_tk->req_len = 32;
        p_req_mem_0_tk->ts = ps_ts;
        jpeg::p_req_mem.pushToken(p_req_mem_0_tk);
        enqueueReq(id_counter++, img_addr+i*32, 32, (int)jpeg::CstStr::DMA_READ, 0);
    }
    img_addr += avg_block_size*size/32*32;
    
    int left = avg_block_size*size - avg_block_size*size/32*32;
    if(left > 0){
        NEW_TOKEN(jpeg::token_class_req_len, p_req_mem_0_tk);
        p_req_mem_0_tk->req_len = left;
        p_req_mem_0_tk->ts = ps_ts;
        jpeg::p_req_mem.pushToken(p_req_mem_0_tk);
        enqueueReq(id_counter++, img_addr, 32, (int)jpeg::CstStr::DMA_READ, 0);
    }
    // for data read end

    for (int i = 0; i < size; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        new_token->ts = ps_ts;
        jpeg::ptasks_bef_header.pushToken(new_token);
    }

    // for data write
    for (uint64_t i = 0; i < decoded_len/4; ++i){
        enqueueReq(id_counter++, (uint64_t)dst+i*4, 4, (int)jpeg::CstStr::DMA_WRITE, 1);
    }

    printf("setup_LPN done\n");
}

