#include <assert.h>
#include <bits/stdint-uintn.h>
#include <sys/types.h>
#include "place_transition.hh"
#include "lpn_funcs.hh"  
#include "places.hh"
#include "transitions.hh"
#include "lpn_init.hh"
#include "config.hh"
#include <stdio.h>

std::deque<token_class_abirrs*> dma_read_requests;
std::deque<token_class_abirrs*> dma_write_requests;
std::deque<token_class_abirrs*> dma_read_resp;
std::deque<token_class_abirrs*> dma_write_resp;

#define T_SIZE 30
#define LOOP_TS(func) for(int i=0;i < T_SIZE; i++){ \
        Transition* t = pcie_t_list[i]; \
        func; \
    }  

Transition* pcie_t_list[T_SIZE] = {&troot_tmpbuf_0_1, &troot_tmpbuf_1_0, &root_tmpbuf_to_sendbuf_0_arbiter, &root_send_0, &root_tmpbuf_to_sendbuf_1_arbiter, &root_send_1, &d1_send, &d1_proc, &d1_arbiter, &d0_send, &d0_proc, &d0_arbiter, &d0_get_read, &d0_get_write, &d0_send_mem_req, &d0_send_cmpl_reply, &d0_mem_read_mem_get, &d0_mem_read_recv_from_mem, &d0_mem_read_mem_put, &d0_mem_write_mem_get, &d0_mem_write_recv_from_mem, &d0_mem_write_mem_put, &root_P2P_d0_transfer, &root_P2P_d0_merge, &d0_P2P_root_transfer, &d0_P2P_root_merge, &root_P2P_d1_transfer, &root_P2P_d1_merge, &d1_P2P_root_transfer, &d1_P2P_root_merge };

uint64_t inflight_read_write = 0;

void pcie_advance_mem(uint64_t ts){
    mem_advance_until_time(ts);
    // int mem_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len)
    uint64_t original_token, id, buffer, len;
    while(mem_get(&original_token, &id, &buffer, &len)){
        inflight_read_write --;
        auto tk = (token_class_abirrs*)original_token;
        if(tk->rw == 0){
            tk->ts = ts;
            // printf("@%ld[ns] dma_read_done id %ld; \n", ts/1000, tk->id);
            dma_read_resp.push_back(tk);
        }else{
            tk->ts = ts;
            // printf("@%ld[ns] dma_write_done id %ld;\n", ts/1000, tk->id);
            dma_write_resp.push_back(tk);
        }
    }

    // mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr
    while(!dma_read_requests.empty()){
        token_class_abirrs* token = dma_read_requests.front();
        dma_read_requests.pop_front();
        // printf("mem put read id %lu\n", token->id);
        mem_put(ts, token->id, token->addr, token->size, 0, 0, 0, (uint64_t)token);
        inflight_read_write++;
    }
    
    while(!dma_write_requests.empty()){
        auto token = dma_write_requests.front();
        dma_write_requests.pop_front();
        // printf("mem put write id %lu\n", token->id);
        mem_put(ts, token->id, token->addr, token->size, 0, 0, 1, (uint64_t)token);
        inflight_read_write++;
    }

}

// this is not necessary to implement for every accelerators
void pcie_advance_until_time(uint64_t ts){
    assert(MPS <= CREDIT*UNIT);
    uint64_t time = 0, prev_time = 0; 
    uint64_t start = 0;
    while(time <= ts){
        while(time <= ts){
            LOOP_TS(trigger(t));
            time = min_time_g(pcie_t_list, T_SIZE);
            uint64_t child_active_time = 0;
            int active = mem_active(&child_active_time);
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
                Transition* t = pcie_t_list[i];
                int fired = sync(t, time);
                // if(fired){
                //     printf("fired %s\n", t->id.c_str());
                // }
            }
            pcie_advance_mem(time);
        }
        if(time == lpn::LARGE){
            uint64_t nx_active_time = 0;
            int active = mem_active(&nx_active_time);
            if(active == 0){
                break;
            }
            // the vta lpn has no events, but mem might have events
            pcie_advance_mem(nx_active_time);
            time = nx_active_time;
        }else{
            break;
        }
    }
    // for (int i = 0; i < T_SIZE; i++){
    //     auto t = pcie_t_list[i];
    //     printf("t %s, t->count %d\n", t->id.c_str(), t->count);
    // }
    
    // for (int i = 0; i < T_SIZE; i++){
    //     auto t = pcie_t_list[i];
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
    // printf(" //// (end %lu, start%lu)pcie lpn advanced this much: %lu[ns]\n", prev_time/1000, start/1000, (prev_time - start)/1000);
}

int pcie_active(uint64_t* next_active_ts){
    uint64_t time = trigger_and_min_time_g(pcie_t_list, T_SIZE);
    uint64_t child_active_time = 0;
    int active = mem_active(&child_active_time);
    if (active == 1 && time > child_active_time){
        time = child_active_time;
    }
    if(time == lpn::LARGE){
        assert(inflight_read_write == 0);
        assert(dma_read_requests.empty() && dma_write_requests.empty() && dma_read_resp.empty() && dma_write_resp.empty());
        return 0;
    }
    *next_active_ts = time;
    return 1;
}

// here it's simplified to check if all transitions are finished
// this is wrong if there are multiple accelerators connected
// put a memory request
int pcie_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr){
    // printf("pcie_put\n");
    // create a new token
    
    NEW_TOKEN(token_class_ralmtra, dataref);
    dataref->req_id = req_id;
    dataref->addr = addr;
    dataref->mem_buffer_ptr = mem_buffer_ptr;
    dataref->dma_fd = dma_fd;
    dataref->type = type;
    dataref->ref_ptr = ref_ptr;
    dataref->subreq_cnt = 0;
    dataref->total_len = len;
    // acquired length
    dataref->len = 0;

    if(type == 0 ){
        //read, need to split the request
        int num_reqs = (int)ceil((len /(double) READMPS));
        int remain = len;
        int offset = 0;
        for (int i = 0; i < num_reqs; ++i) {
            auto this_size = std::min(READMPS, remain);
            remain -= this_size;
            NEW_TOKEN(token_class_ddopst, pcie_token);
            pcie_token->src = 1;
            pcie_token->dst = 0;
            pcie_token->type = type;
            pcie_token->pkt_size = 16;
            pcie_token->requested_size = this_size;
            pcie_token->offset = offset;
            pcie_token->dataref = (uint64_t)dataref;
            pcie_token->ts = virtual_ts;
            d1_reqbuf_nonpost.pushToken(pcie_token);
            offset = (offset + this_size);
        }
    }else{
        NEW_TOKEN(token_class_ddopst, pcie_token);
        pcie_token->src = 1;
        pcie_token->dst = 0;
        pcie_token->type = type;
        pcie_token->pkt_size = len;
        pcie_token->requested_size = len;
        d1_reqbuf_post.pushToken(pcie_token);
        pcie_token->offset = 0;
        pcie_token->dataref = (uint64_t)dataref;
        pcie_token->ts = virtual_ts;
    }
  
    return 0;
}

// get a memory request
int pcie_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len){
    while(1){
        if(d1_recvcmpl.tokens.size() == 0){
            return 0;
        }
        auto token = d1_recvcmpl.tokens[0];
        d1_recvcmpl.popToken();
        auto dataref = (token_class_ralmtra*)(token->dataref);
        assert(dataref!=NULL);
        dataref->len += token->requested_size;
        // printf("pcie_get type %d, %ld %ld\n", dataref->type, dataref->len, dataref->total_len);
        // received all chunks of the request
        if(dataref->len == dataref->total_len){
            //a complete request
            *ref_ptr = (dataref->ref_ptr);
            *req_id = (dataref->req_id);
            if(buffer == NULL){
                if(token->type == 1){
                    *len = 0;
                }else{
                    *len = (dataref->total_len);
                }
            }else{
                if(token->type == 1){
                    *buffer = 0;
                    *len = 0;
                }else{
                    *buffer = (dataref->mem_buffer_ptr);
                    *len = (dataref->total_len);
                }
            }
            // printf("successful pcie_get\n");
            return 1;
        }
    }
}

void pcie_stats(){
    printf("EMPTY PCIe stats\n");
}
