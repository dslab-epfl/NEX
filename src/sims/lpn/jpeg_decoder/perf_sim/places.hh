#pragma once
#include "sims/lpn/lpn_common/place_transition.hh"
#include "token_types.hh"

extern Place<token_class_avg_block_size> pvar_recv_token;
extern Place<> pstart;
extern Place<token_class_iasbrr> dma_read_req_get_0;
extern Place<> p4;
extern Place<token_class_idx> dma_read_fifo_order;
extern Place<> p0;
extern Place<> dma_read_SINK;
extern Place<> p1;
extern Place<> dma_read_send_cap;
extern Place<> p2;
extern Place<> p3;
extern Place<> p6;
extern Place<> p7;
extern Place<> p8;
extern Place<> p20;
extern Place<token_class_iasbrr> dma_write_recv_buf;
extern Place<token_class_iasbrr> dma_write_req_put_0;
extern Place<> p21;
extern Place<token_class_iasbrr> dma_write_mem_put_buf;
extern Place<> p22;
extern Place<token_class_iasbrr> dma_write_req_get_0;
extern Place<> pdone;
extern Place<token_class_idx> dma_write_fifo_order;
extern Place<token_class_nonzero> pvarlatency;
extern Place<> dma_write_SINK;
extern Place<token_class_req_len> p_req_mem;
extern Place<> p_recv_buf;
extern Place<> dma_write_send_cap;
extern Place<token_class_iasbrr> dma_read_mem_put_buf;
extern Place<token_class_num> p_send_dma;
extern Place<token_class_iasbrr> dma_read_recv_buf;
extern Place<token_class_iasbrr> dma_read_req_put_0;
extern Place<> ptasks_bef_header;
extern Place<> ptasks_aft_header;