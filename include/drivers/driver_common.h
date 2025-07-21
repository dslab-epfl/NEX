#ifndef  __DRIVER_COMMON__
#define __DRIVER_COMMON__
#include <stdint.h>
#include <stdlib.h>
#include <drivers/jpegdecoder/jpeg_decoder_regs.h>
#include <drivers/vta/regs.h>
#include <drivers/protoacc/regs.h>
#include <drivers/sched/regs.h>

#define BPF_SCHED 0
#define JPEG_DECODER 1
#define VTA 2
#define PROTOACC 3

uintptr_t driver_initialize();
uintptr_t open_shm_for_nex(const char* shm_path);
#endif