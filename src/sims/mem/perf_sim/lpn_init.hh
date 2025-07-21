
#pragma once
#include "places.hh"
#include <bits/stdint-uintn.h>
#include <stdint.h>

extern "C" {
    void mem_lpn_init();
    int mem_put(uint64_t dev_id, uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr);
    int mem_get(uint64_t dev_id, uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len, uint64_t* ts);
    void mem_advance_until_time(uint64_t ts);
    int mem_active(uint64_t* next_active_ts);
    void mem_stats();
}

void mem_lpn_init() {
  for (int i = 0; i < 20; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        memory_inflight_cap.pushToken(new_token);
    }

}