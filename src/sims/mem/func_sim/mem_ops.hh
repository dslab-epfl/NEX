#pragma once
#include <cstddef>
#include <cstdio>
#include <stdint.h>
#include <unistd.h>

void mem_read(int dma_fd, uint64_t addr, void *buffer, size_t len);

void mem_write(int dma_fd, uint64_t addr, const void *buffer, size_t len);