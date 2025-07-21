#include <stdlib.h>
#include <stdint.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#define dprintf
#define BUF_SIZE 8192 * 2

static size_t last_idx = 0;
static size_t last_buf_size = 0; 
static uint8_t* buffer = nullptr;
static uint8_t* new_buffer = nullptr;

uint8_t* GetGlobalBuffer(size_t len){
    if(buffer==nullptr){
        buffer = static_cast<uint8_t*>(malloc(sizeof(uint8_t)*len));
    }
    return buffer;
}

int CheckNotEnoughBuf(size_t future_idx, size_t len, const uint8_t* buf){
    //future_idx is accessed
    return 0;
   
}

void CheckPointIdx(int cur) {
    last_idx = cur;
    dprintf("checkidx %zu \n", last_idx);
}

// void CheckPointIdx(int cur, size_t len, uint8_t* buf) {
//     last_idx = cur;
//     uint8_t* last_buf = GetGlobalBuffer();
//     for(int i = last_idx; i < len; i++){
//         last_buf[i-last_idx] = buf[i];
//     }
//     last_buf_size = len - last_idx;
//     dprintf("Checkpointing, len %d, checked-idx %d \n", len, last_idx);
// }

void RollLog(){
    dprintf("last_idx %d, last_buf_size %d \n", last_idx, last_buf_size);
}

#define CHECK_ENOUGH_BUF(future, len, buf, ret)\
    if(CheckNotEnoughBuf(future, len, buf)){ \
        dprintf(#future "<" #len "\n"); \
        return ret; \
    }


void RollbackBufReset(){
    last_buf_size = 0;
    last_idx = 0;
    free(buffer);
    free(new_buffer);
    buffer = nullptr;
    new_buffer = nullptr;
}

