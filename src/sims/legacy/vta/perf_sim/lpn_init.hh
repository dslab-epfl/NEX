#pragma once
#include "places.hh"
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

// this is not necessary to implement for every accelerators
extern "C" {
    void vta_lpn_init();
    void vta_sim_start( uint64_t, uint32_t, uint32_t, uint64_t);
    int vta_perf_sim_finished();
    void vta_advance_until_time(uint64_t ts);
}

void vta_lpn_init() {
    
    setupReqQueues(ids);
   
    {
        for(int i=0; i < 1; i++){
            NEW_TOKEN(EmptyToken, new_token);
            vta::pload_ctrl.pushToken(new_token);
        
        }
    }

    {
        NEW_TOKEN(EmptyToken, new_token);
        vta::pcompute_ctrl.pushToken(new_token);
    }

    {
        for(int i=0; i < 1; i++){
            NEW_TOKEN(EmptyToken, new_token);
            vta::pstore_ctrl.pushToken(new_token);
        }
    }

    for (int i = 0; i < 512; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        vta::pcompute_cap.pushToken(new_token);
    }

    for (int i = 0; i < 512; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        vta::pload_cap.pushToken(new_token);
    }

    for (int i = 0; i < 512; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        vta::pstore_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        vta::pcontrol.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        vta::dma_write_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        vta::dma_read_send_cap.pushToken(new_token);
    }
}
