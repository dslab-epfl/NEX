#pragma once
#include <stdint.h>
#include <drivers/driver_common.h>
#include <config/config.h>

// for mem read/write
extern int mem_active(uint64_t* next_ts);
extern void mem_lpn_init();
extern void mem_advance_until_time(uint64_t ts);
extern int mem_put(uint64_t dev_id, uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int pid_fd, int type, uint64_t ref_ptr);
extern int mem_get(uint64_t dev_id, uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len, uint64_t* virtual_ts);
extern void mem_stats();

#define EXTERN_LPN_FUNC(name) \
  extern void name##_lpn_init(); \
  extern void name##_advance_until_time(uint64_t ts); \
  extern int name##_perf_sim_finished();

#if CONFIG_PCIE_LPN
  EXTERN_LPN_FUNC(vta);
  EXTERN_LPN_FUNC(jpeg);
  extern void vta_sim_start(uint64_t insn_phy_addr, uint64_t base_addr, uint32_t insn_count, uint32_t wait_cycles, uint64_t ts);
  extern void jpeg_sim_start(uint8_t* buf, uint32_t len, uint8_t* dst, uint64_t* decoded_len, uint64_t ts);
  extern void pcie_lpn_init();
  extern void pcie_advance_until_time(uint64_t ts);
#endif

#if CONFIG_JPEG_LPN
  EXTERN_LPN_FUNC(jpeg);
  extern void jpeg_sim_start(uint8_t* buf, uint32_t len, uint8_t* dst, uint64_t* decoded_len, uint64_t ts);
#endif 

#if CONFIG_VTA_LPN
  EXTERN_LPN_FUNC(vta);
  extern void vta_sim_start(uint64_t insn_phy_addr, uint64_t base_addr, uint32_t insn_count, uint32_t wait_cycles, uint64_t ts);
#endif

int lpn_inactive();

int perf_sim_finished(int type);

void advance_until_time(int type, uint64_t ts);

#if CONFIG_PROTOACC_LPN
  EXTERN_LPN_FUNC(protoacc);
  extern void protoacc_sim_start(int pid, uint64_t mem_base, uint64_t msg_start_addr, uint64_t serialized_msg_addr, uint64_t descriptor_table_addr, uint64_t src_base_addr, uint64_t ts);
#endif