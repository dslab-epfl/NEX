#pragma once
#include <stdint.h>

#define CTRL_REG_START_BIT 0x80000000
#define CTRL_REG_DONE_BIT 0x60000000
#define CTRL_REG_LEN_MASK 0x00FFFFFF

// control registers for the JPEG decoder, based on
// https://github.com/ultraembedded/core_jpeg_decoder
typedef struct __attribute__((packed)) JpegRegs {
  uint32_t ctrl; //0x0
  uint32_t isBusy; //0x4
  uint64_t src; //0x8
  uint64_t dst; //0x10
} JpegRegs;