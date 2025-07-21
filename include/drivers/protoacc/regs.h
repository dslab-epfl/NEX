#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)) ProtoAccRegs {
  uint64_t ctrl; // 0x00
  uint64_t completed_msg; // 0x08
  uint64_t stringobj_output_addr; // 0x10
  uint64_t string_ptr_output_addr; // 0x18
  uint64_t hasbits_offset; // 0x20
  uint64_t min_max_fieldno; // 0x28
  uint64_t descriptor_table_ptr; // 0x30
  uint64_t src_base_addr; // 0x38
  uint64_t mem_base; // 0x40 mmap region base address
} ProtoAccRegs;