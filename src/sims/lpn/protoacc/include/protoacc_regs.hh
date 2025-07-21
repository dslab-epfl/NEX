#pragma once
#include <bits/stdint-uintn.h>
#include <cstdint>

// WriteMappedReg(acc_regs, 0x8, min_fieldno);
// WriteMappedReg(acc_regs, 0xc, max_fieldno);
// WriteMappedReg(acc_regs, 0x10, new_descriptor_table_addr & 0xffffffff);
// WriteMappedReg(acc_regs, 0x14, (new_descriptor_table_addr >> 32)& 0xffffffff);
// WriteMappedReg(acc_regs, 0x18, new_src_base_addr & 0xffffffff);
// WriteMappedReg(acc_regs, 0x1c, (new_src_base_addr >> 32) & 0xffffffff);
// WriteMappedReg(acc_regs, 0x20, hasbits_offset & 0xffffffff);
// WriteMappedReg(acc_regs, 0x24, (hasbits_offset >> 32) & 0xffffffff);
// WriteMappedReg(acc_regs, 0x28, stringalloc_region_ptr_as_int_tail & 0xffffffff);
// WriteMappedReg(acc_regs, 0x2c, (stringalloc_region_ptr_as_int_tail >> 32)& 0xffffffff);
// WriteMappedReg(acc_regs, 0x30, string_ptr_region_ptr_as_int & 0xffffffff);
// WriteMappedReg(acc_regs, 0x34, (string_ptr_region_ptr_as_int >> 32) & 0xffffffff);

struct __attribute__((packed)) PACRegs {
  uint32_t ctrl; // 0x0
  uint32_t completed_msg; // 0x4
  uint32_t min_fieldno; // 0x8
  uint32_t max_fieldno; // 0xc
  uint32_t descriptor_table_addr_l; // 0x10
  uint32_t descriptor_table_addr_h; // 0x14
  uint32_t src_base_addr_l; // 0x18
  uint32_t src_base_addr_h; // 0x1c
  uint32_t hasbits_offset_l; // 0x20
  uint32_t hasbits_offset_h; // 0x24
  uint32_t stringalloc_region_ptr_as_int_tail_l; // 0x28
  uint32_t stringalloc_region_ptr_as_int_tail_h; // 0x2c
  uint32_t string_ptr_region_ptr_as_int_l; // 0x30
  uint32_t string_ptr_region_ptr_as_int_h; // 0x34
};

struct __attribute__((packed)) VerilatorRegs {
  // activates or deactivates tracing
  bool tracing_active;
};
