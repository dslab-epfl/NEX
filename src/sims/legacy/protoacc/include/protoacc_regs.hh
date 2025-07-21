#pragma once
#include <cstdint>

#define CTRL_REG_START_BIT 0x80000000
#define CTRL_REG_LEN_MASK 0x00FFFFFF

// control registers for the VTA, based on
// 3rdparty/vta-hw/src/simbricks-pci/pci_driver.cc
struct __attribute__((packed)) VTARegs {
    uint32_t status; // 0
    uint32_t _0x04;  // 4
    uint32_t insn_count; // 8
    uint32_t insn_phy_addr_lh; // 12
    uint32_t insn_phy_addr_hh; // 16
    uint32_t _0x14; // 20
    uint32_t _0x18; // 24
    uint32_t _0x1c; // 28
    uint32_t _0x20; // 32
};

struct __attribute__((packed)) VerilatorRegs {
  // activates or deactivates tracing
  bool tracing_active;
};
