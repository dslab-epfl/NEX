#pragma once
#include "places.hh"
#include <nlohmann/json.hpp>
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

// this is not necessary to implement for every accelerators
extern "C" {
    void protoacc_lpn_init();
    void protoacc_sim_start(int mem_fd, uint64_t mem_base, uint64_t stringobj_output_addr, uint64_t string_ptr_output_addr, uint64_t descriptor_table_addr, uint64_t src_base_addr, uint64_t ns_ts);
    int protoacc_perf_sim_finished(uint64_t* finished_task);
    void protoacc_advance_until_time(uint64_t ts);
}

void protoacc_lpn_init() {
    
    setupReqQueues(ids);
    
    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f2_p_S_WAIT_CMD.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f2_p_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f6_p_dist_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f6_p_S_WAIT_CMD.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f6_p_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        pwrite_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        frontend_pAdvance_OK.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        frontend_psWaitForRequest.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        frontend_pholder_split_msg.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(token_class_field_index, frontend_pwrite_index_holder_tk);
        frontend_pwrite_index_holder_tk->field_index = 1;
        frontend_pwrite_index_holder.pushToken(frontend_pwrite_index_holder_tk);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        frontend_phold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(token_class_field_index, pdispatch_index_holder_tk);
        pdispatch_index_holder_tk->field_index = 1;
        pdispatch_index_holder.pushToken(pdispatch_index_holder_tk);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        pdispatch_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f3_p_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f1_p_dist_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f1_p_S_WAIT_CMD.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f1_p_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f5_p_dist_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f5_p_S_WAIT_CMD.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f4_p_dist_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f5_p_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f4_p_S_WAIT_CMD.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f3_p_dist_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f4_p_hold.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f3_p_S_WAIT_CMD.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        f2_p_dist_hold.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        dma_write_port_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        dma_read_port_send_cap.pushToken(new_token);
    }
}
