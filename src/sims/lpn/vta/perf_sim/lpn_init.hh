#pragma once
#include "places.hh"
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

// this is not necessary to implement for every accelerators
extern "C" {
    void lpn_init();
}

void lpn_init() {
  

    {
        NEW_TOKEN(EmptyToken, new_token);
        pload_ctrl.pushToken(new_token);
    }

    {
        NEW_TOKEN(EmptyToken, new_token);
        pcompute_ctrl.pushToken(new_token);
    }

    {
        NEW_TOKEN(EmptyToken, new_token);
        pstore_ctrl.pushToken(new_token);
    }
    
    for (int i = 0; i < 512; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        pcompute_cap.pushToken(new_token);
    }

    for (int i = 0; i < 512; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        pload_cap.pushToken(new_token);
    }

    for (int i = 0; i < 512; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        pstore_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        pcontrol.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        dma_write_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        dma_read_send_cap.pushToken(new_token);
    }
}
