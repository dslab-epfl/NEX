#pragma once
#include <bits/stdint-uintn.h>
#include <stdint.h>
#include "places.hh"
#include "config.hh"

extern "C" {
    void pcie_lpn_init();
    int pcie_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr);
    int pcie_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len);
    void pcie_advance_until_time(uint64_t ts);
    int pcie_active(uint64_t* next_active_ts);
    void pcie_stats();
}

void pcie_lpn_init() {
    for (int i = 0; i < MEM_N_PARALLEL; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_mem_req_max_inflight.pushToken(new_token);
    }

    for (int i = 0; i < CREDIT; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        root_recvcap_0.pushToken(new_token);
    }

    for (int i = 0; i < REQ_BUF_CAP; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_reqbuf_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d1_reqbuf_post_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d1_reqbuf_nonpost_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_reqbuf_cmpl_cap.pushToken(new_token);
    }

    for (int i = 0; i < CREDIT; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d1_recvcap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d1_reqbuf_cmpl_cap.pushToken(new_token);
    }

    for (int i = 0; i < CREDIT; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        root_recvcap_1.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(token_class_idx, root_tmpbuf_to_sendbuf_1_pidx_tk);
        root_tmpbuf_to_sendbuf_1_pidx_tk->idx = 0;
        root_tmpbuf_to_sendbuf_1_pidx.pushToken(root_tmpbuf_to_sendbuf_1_pidx_tk);
    }

    for (int i = 0; i < REQ_BUF_CAP; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d1_reqbuf_cap.pushToken(new_token);
    }

    for (int i = 0; i < MEM_N_PARALLEL; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_mem_write_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_reqbuf_post_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_reqbuf_nonpost_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(token_class_idx, d1_pidx_tk);
        d1_pidx_tk->idx = 0;
        d1_pidx.pushToken(d1_pidx_tk);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(token_class_idx, root_tmpbuf_to_sendbuf_0_pidx_tk);
        root_tmpbuf_to_sendbuf_0_pidx_tk->idx = 0;
        root_tmpbuf_to_sendbuf_0_pidx.pushToken(root_tmpbuf_to_sendbuf_0_pidx_tk);
    }

    for (int i = 0; i < CREDIT; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_recvcap.pushToken(new_token);
    }

    for (int i = 0; i < MEM_N_PARALLEL; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        d0_mem_read_send_cap.pushToken(new_token);
    }

    for (int i = 0; i < 1; ++i){
        NEW_TOKEN(token_class_idx, d0_pidx_tk);
        d0_pidx_tk->idx = 0;
        d0_pidx.pushToken(d0_pidx_tk);
    }

}