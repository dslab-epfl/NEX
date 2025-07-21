#include "mem_ops.hh"
#include <assert.h>
#define DEBUG_PRINTS(...) printf(__VA_ARGS__)

void mem_read(int pid_fd, uint64_t addr, void *buffer, size_t len) {
    if(buffer == NULL){
        return;
    }
    // Simulated DMA read
    // printf("Simulated DMA read from addr: %p, size %ld \n", (void*)addr, len);
    // assert(0);
    ssize_t nread = pread(pid_fd, buffer, len, addr);
    // for(int i = 0; i < nread; i++) {
        // DEBUG_PRINTS("%02x", ((unsigned char*)buffer)[i]);
    // }
    // DEBUG_PRINTS("\n");
}

void mem_write(int pid_fd, uint64_t addr, const void *buffer, size_t len) {
    if(buffer == NULL){
        return;
    }
    // Simulated DMA write
    // DEBUG_PRINTS("Simulated DMA write to addr: %lu, size %ld\n", addr, len);
    // ssize_t nwrite = pwrite(pid_fd, buffer, len, addr);
    // if(nwrite != len) {
    //     printf("nwrite %ld and len %ld\n", nwrite, len);
    // }
    // assert(nwrite == len);
}