#include <assert.h>
#include <bits/stdint-uintn.h>
#include <sys/types.h>
#include <chrono>
#include "lpn_funcs.hh"  
#include "places.hh"
#include "transitions.hh"
#include "lpn_init.hh"
#include "sims/lpn/jpeg_decoder/include/driver.hh"
#include "sims/lpn/jpeg_decoder/include/lpn_req_map.hh"
#include "sims/lpn/lpn_common/place_transition.hh"
#include "sims/lpn/jpeg_decoder/include/driver.hh"
#include "transitions.hh"
#include "places.hh"

#define T_SIZE 20

Transition* t_list[T_SIZE] =  {&dma_read_arbiter, &dma_read_mem_get, &dma_read_recv_from_mem, &dma_read_mem_put, &dma_write_arbiter, &dma_write_mem_get, &dma_write_recv_from_mem, &dma_write_mem_put, &twait_header, &t2, &t3, &t4, &t5, &t0, &t1, &twait_header, &tinp_fifo, &toutput_fifo, &t_input_fifo_recv, &toutput_fifo_recv };
extern "C" {
    extern void jpeg_lpn_driver(uint8_t* buf, uint32_t len, uint8_t* dst, uint64_t* decoded_len, int** ret_array_ptr, int* size);
}

void lpn_start(uint64_t buf, uint32_t len, uint64_t dst, uint64_t ps_ts){

    int* non_zeros;
    int size;
    int size_of_img = len;
    uint64_t img_addr = (uint64_t)buf;
    uint64_t decoded_len;

    // 50MB
    uint8_t* constrcuted_buf = (uint8_t*)malloc(1024*1024*50);
    // this runs func-sim to completion and returns the features
    uint8_t* retrived_buf = (uint8_t*)malloc(len+ZC_DMA_BLOCK_SIZE);
    printf("size of img %ld\n", len, ZC_DMA_BLOCK_SIZE);
    for(uint32_t i = 0; i < len/ZC_DMA_BLOCK_SIZE + 1; i++){
        uint64_t address = img_addr + i*ZC_DMA_BLOCK_SIZE;
        auto dma_op = std::make_unique<JPEGDmaReadOp<ZC_DMA_BLOCK_SIZE>>(address, ZC_DMA_BLOCK_SIZE, 0);
        auto return_dma = jpeg_bm_->ZeroCostBlockingDma(std::move(dma_op));
        memcpy(retrived_buf+i*ZC_DMA_BLOCK_SIZE, return_dma->data, ZC_DMA_BLOCK_SIZE);
        std::cerr << "Read from " << address << " to " << address+ZC_DMA_BLOCK_SIZE << std::endl;
    }
    std::cerr<< "start jpeg lpn driver \n" << std::endl;
   
    jpeg_lpn_driver(retrived_buf, len+ZC_DMA_BLOCK_SIZE, constrcuted_buf, &decoded_len, &non_zeros, &size);
    // this runs func-sim to completion and returns the features
    printf("func_sim: should have %d blocks\n", size*4/6);
   
    for (int i = 0; i < size; ++i){
        NEW_TOKEN(token_class_nonzero, pvarlatency_0_tk);
        pvarlatency_0_tk->nonzero = non_zeros[i];
        pvarlatency_0_tk->ts = ps_ts;
        pvarlatency.pushToken(pvarlatency_0_tk);
    }

    int size_of_data = size_of_img - 18*32;
    int avg_block_size = size_of_data/size;

    printf("total blocks : %d; and average block size %d\n", size, avg_block_size);
    
    pvar_recv_token.reset();
    NEW_TOKEN(token_class_avg_block_size, pvar_recv_token_0_tk);
    pvar_recv_token_0_tk->avg_block_size = avg_block_size;
    pvar_recv_token.pushToken(pvar_recv_token_0_tk);

    uint64_t id_counter = 0;
    // for header read
    for(int i = 0; i < 18; i++){
        NEW_TOKEN(token_class_req_len, p_req_mem_0_tk);
        p_req_mem_0_tk->req_len = 32;
        p_req_mem_0_tk->ts = ps_ts;
        p_req_mem.pushToken(p_req_mem_0_tk);
        enqueueReq(id_counter++, img_addr+i*32, 32, (int)jpeg::CstStr::DMA_READ, 0, 0);
    }
    // for header read end
    img_addr += 18*32;

    // for data read
    for(int i = 0; i < avg_block_size*size/32; i++){
        NEW_TOKEN(token_class_req_len, p_req_mem_0_tk);
        p_req_mem_0_tk->req_len = 32;
        p_req_mem_0_tk->ts = ps_ts;
        p_req_mem.pushToken(p_req_mem_0_tk);
        enqueueReq(id_counter++, img_addr+i*32, 32, (int)jpeg::CstStr::DMA_READ, 0, 0);
    }
    img_addr += avg_block_size*size/32*32;
    
    int left = avg_block_size*size - avg_block_size*size/32*32;
    if(left > 0){
        NEW_TOKEN(token_class_req_len, p_req_mem_0_tk);
        p_req_mem_0_tk->req_len = left;
        p_req_mem_0_tk->ts = ps_ts;
        p_req_mem.pushToken(p_req_mem_0_tk);
        enqueueReq(id_counter++, img_addr, 32, (int)jpeg::CstStr::DMA_READ, 0, 0);
    }
    // for data read end

    for (int i = 0; i < size; ++i){
        NEW_TOKEN(EmptyToken, new_token);
        new_token->ts = ps_ts;
        ptasks_bef_header.pushToken(new_token);
    }

    // for data write
    for (uint64_t i = 0; i < decoded_len/4; ++i){
        enqueueReq(id_counter++, (uint64_t)dst+i*4, 4, (int)jpeg::CstStr::DMA_WRITE, 1, constrcuted_buf+i*4);
    }

    printf("setup_LPN done\n");
}

// void lpn_reset(){
//     ClearReqQueues(ids);
// }

int lpn_finished(){
    if(!(dma_read_requests.empty() && dma_write_requests.empty() && dma_read_resp.empty() && dma_write_resp.empty())){
        // std::cerr << "dma_read_requests size " << dma_read_requests.size() << std::endl;
        // std::cerr << "dma_write_requests size " << dma_write_requests.size() << std::endl;
        // std::cerr << "dma_read_resp size " << dma_read_resp.size() << std::endl;
        // std::cerr << "dma_write_resp size " << dma_write_resp.size() << std::endl;
        return 0;
    }
    uint64_t next_ts = NextCommitTime(t_list, T_SIZE);
    if(next_ts == lpn::LARGE){
        return 1;
    }
    return 0;
}