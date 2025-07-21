#pragma once
#include "places.hh"
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

// this is not necessary to implement for every accelerators
extern "C" {
    void lpn_init();
}

void lpn_init() {
   for (int i = 0; i < 4; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        p4.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        dma_read_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 4; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        p6.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        p8.pushToken(new_token);
    }

    for (int i = 0; i < 4; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        p20.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        dma_write_send_cap.pushToken(new_token);
    }
}
