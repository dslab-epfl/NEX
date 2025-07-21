#pragma once
#include <cstddef>
#include <cstdio>
#include <stdint.h>
#include <unistd.h>

void mem_read(int pid_fd, uint64_t addr, void *buffer, size_t len);

void mem_write(int pid_fd, uint64_t addr, const void *buffer, size_t len);