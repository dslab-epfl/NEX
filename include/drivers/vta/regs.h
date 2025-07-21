#pragma once
#include <stdint.h>

// control registers for the VTA, based on
// 3rdparty/vta-hw/src/simbricks-pci/pci_driver.cc
typedef struct __attribute__((packed)) VTARegs {
    uint32_t _0x00; // 0
    uint32_t _0x04;  // 4
    uint32_t _0x08; // 8
    union{
      uint32_t insn_phy_addr_lh; // 12
      uint32_t _0x0c; // 12
    };
    union {
      uint32_t insn_phy_addr_hh; // 16
      uint32_t _0x10; // 16
    };
    uint32_t _0x14; // 20
    uint32_t _0x18; // 24
    uint32_t _0x1c; // 28
    uint32_t _0x20; // 32
    uint64_t _0x24; // 36
} VTARegs;