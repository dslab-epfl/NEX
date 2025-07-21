#pragma once
#include "places.hh"
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

// this is not necessary to implement for every accelerators
extern "C" {
    void jpeg_lpn_init();
    void jpeg_sim_start(uint8_t* buf, uint32_t len, uint8_t* dst, uint64_t decoded_len, uint64_t ts);
    int jpeg_perf_sim_finished();
    void jpeg_advance_until_time(uint64_t ts);
}

void jpeg_lpn_init() {
   for (int i = 0; i < 4; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        jpeg::p4.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        jpeg::dma_read_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 4; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        jpeg::p6.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        jpeg::p8.pushToken(new_token);
    }

    for (int i = 0; i < 4; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        jpeg::p20.pushToken(new_token);
    }

    for (int i = 0; i < 16; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        jpeg::dma_write_send_cap.pushToken(new_token);
    }
}
