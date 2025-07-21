/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file sim_driver.cc
 * \brief VTA driver for simulated backend.
 */
#include <assert.h>
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <unordered_map>

#include "../include/vta/hw_spec.h"
#include "sims/lpn/vta/include/driver.hh"
#include "sims/lpn/vta/include/lpn_req_map.hh"
#include "sims/lpn/vta/include/vta/driver.h"
#include "sims/lpn/vta/perf_sim/token_types.hh"
#include "sims/lpn/vta/perf_sim/places.hh"

// #define DEBUG_FUNC_SIM

namespace vta {
namespace sim {

uint64_t id_counter = 0;

ssize_t read_sim_memory(uintptr_t base_address, void *buffer, size_t size, int tag) {
   //log out to a file 
    if(tag != -1){
      enqueueReq(id_counter++, base_address, size, tag, 0, 0);
    }

    // this runs func-sim to completion and returns the features
    uint8_t* retrived_buf = (uint8_t*)buffer;
    uint64_t obtained_size = 0;
    for(uint32_t i = 0; i < size/ZC_DMA_BLOCK_SIZE + 1; i++){
        uint64_t address = base_address + i*ZC_DMA_BLOCK_SIZE;
        auto dma_op = std::make_unique<VTADmaReadOp<ZC_DMA_BLOCK_SIZE>>(address, ZC_DMA_BLOCK_SIZE, tag);
        auto return_dma = vta_bm_->ZeroCostBlockingDma(std::move(dma_op));
        if(size - obtained_size < ZC_DMA_BLOCK_SIZE){
            memcpy(retrived_buf+i*ZC_DMA_BLOCK_SIZE, return_dma->data, size - obtained_size);
            break;
        }
        memcpy(retrived_buf+i*ZC_DMA_BLOCK_SIZE, return_dma->data, ZC_DMA_BLOCK_SIZE);
        obtained_size += ZC_DMA_BLOCK_SIZE;
    }

    return 0;
}

ssize_t write_sim_memory(uintptr_t base_address, void *buffer, size_t size, int tag){
    enqueueReq(id_counter++, base_address, size, tag, 1, buffer);
    auto dma_op = std::make_unique<VTADmaWriteOp>(base_address, size, tag);
    std::memcpy(dma_op->buffer, buffer, size);
    vta_bm_->ZeroCostBlockingDma(std::move(dma_op));
    return 0;
}

/*! \brief debug flag for skipping computation */
enum DebugFlagMask { kSkipExec = 1 };

/*!
 * \brief Helper class to pack and unpack bits
 *  Applies truncation when pack to low level bits.
 *
 * \tparam bits The number of bits in integer.
 * \note This implementation relies on little endian.
 */
template <uint32_t bits>
class BitPacker {
 public:
  explicit BitPacker(void* data) {
    data_ = static_cast<uint32_t*>(data);
  }

  uint32_t GetUnsigned(uint32_t index) const {
    if (bits == 32) {
      return data_[index];
    } else if (bits == 16) {
      return reinterpret_cast<uint16_t*>(data_)[index];
    } else if (bits == 8) {
      return reinterpret_cast<uint8_t*>(data_)[index];
    } else {
      uint32_t offset = index / kNumPackElem;
      uint32_t shift = index % kNumPackElem;
      return (data_[offset] >> shift) & kMask;
    }
  }

  int32_t GetSigned(uint32_t index) const {
    if (bits == 32) {
      return reinterpret_cast<int32_t*>(data_)[index];
    } else if (bits == 16) {
      return reinterpret_cast<int16_t*>(data_)[index];
    } else if (bits == 8) {
      return reinterpret_cast<int8_t*>(data_)[index];
    } else {
      uint32_t offset = index / kNumPackElem;
      uint32_t shift = (index % kNumPackElem) * bits;
      int32_t uvalue = static_cast<int32_t>((data_[offset] >> shift) & kMask);
      int kleft = 32 - bits;
      return (uvalue << kleft) >> kleft;
    }
  }

  void SetUnsigned(uint32_t index, uint32_t value) {
    if (bits == 32) {
      data_[index] = value;
    } else if (bits == 16) {
      reinterpret_cast<uint16_t*>(data_)[index] = value;
    } else if (bits == 8) {
      reinterpret_cast<uint8_t*>(data_)[index] = value;
    } else {
      uint32_t offset = index / kNumPackElem;
      uint32_t shift = (index % kNumPackElem) * bits;
      data_[offset] &= (~(kMask << shift));
      data_[offset] |= (value & kMask) << shift;
    }
  }

  void SetSigned(uint32_t index, int32_t value) {
    if (bits == 32) {
      reinterpret_cast<int32_t*>(data_)[index] = value;
    } else if (bits == 16) {
      reinterpret_cast<int16_t*>(data_)[index] = value;
    } else if (bits == 8) {
      reinterpret_cast<int8_t*>(data_)[index] = value;
    } else {
      uint32_t offset = index / kNumPackElem;
      uint32_t shift = (index % kNumPackElem) * bits;
      data_[offset] &= (~(kMask << shift));
      data_[offset] |= static_cast<uint32_t>(value & kMask) << shift;
    }
  }

 private:
  uint32_t* data_;
  static constexpr uint32_t kNumPackElem = 32 / bits;
  static constexpr uint32_t kMask = (1U << (bits >= 32U ? 31U : bits)) - 1U;
};

/*!
 * \brief Register file.
 * \tparam kBits Number of bits of one value.
 * \tparam kLane Number of lanes in one element.
 * \tparam kMaxNumElem Maximum number of element.
 */
template <int kBits, int kLane, int kMaxNumElem>
class SRAM {
 public:
  /*! \brief Bytes of single vector element */
static const int kElemBytes = (kBits * kLane + 7) / 8;
  /*! \brief content data type */
  using DType = typename std::aligned_storage<kElemBytes, kElemBytes>::type;
  SRAM() {
    data_ = new DType[kMaxNumElem];
  }
  ~SRAM() {
    delete[] data_;
  }
  // Get the i-th index
  void* BeginPtr(uint32_t index) {
    CHECK_LT(index, kMaxNumElem);
    return &(data_[index]);
  }
  // Execute the load instruction on this SRAM
  int Load(const VTAMemInsn* op, uint64_t* load_counter, bool skip_exec,
           uint64_t tag) {
    load_counter[0] += (op->x_size * op->y_size) * kElemBytes;
    if (skip_exec){
      assert(0);
      return 0;
    }

    DType* sram_ptr = data_ + op->sram_base;
    uint8_t* dram_ptr = reinterpret_cast<uint8_t*>(op->dram_base * kElemBytes);

    uint64_t xtotal = op->x_size + op->x_pad_0 + op->x_pad_1;
    uint32_t ytotal = op->y_size + op->y_pad_0 + op->y_pad_1;
    uint64_t sram_end = op->sram_base + xtotal * ytotal;
    CHECK_LE(sram_end, kMaxNumElem);
    memset(sram_ptr, 0, kElemBytes * xtotal * op->y_pad_0);
    sram_ptr += xtotal * op->y_pad_0;

    for (uint32_t y = 0; y < op->y_size; ++y) {
      // Enqueue Request
      memset(sram_ptr, 0, kElemBytes * op->x_pad_0);
      sram_ptr += op->x_pad_0;

      // insert DRAM req

      uint8_t buffer[kElemBytes * op->x_size];
      read_sim_memory((uint64_t)dram_ptr, buffer, kElemBytes * op->x_size, tag);

      memcpy(sram_ptr, buffer, kElemBytes * op->x_size);
      sram_ptr += op->x_size;
      memset(sram_ptr, 0, kElemBytes * op->x_pad_1);
      sram_ptr += op->x_pad_1;
      dram_ptr += kElemBytes * op->x_stride;
    }
    memset(sram_ptr, 0, kElemBytes * xtotal * op->y_pad_1);

    return 0;
  }

  // This is for load 8bits to ACC only
  int Load_int8(const VTAMemInsn* op, uint64_t* load_counter, bool skip_exec,
                uint64_t tag) {

    assert(0);
    /*CHECK_EQ(kBits, VTA_ACC_WIDTH);

    // TODO(zhanghao): extend to other width
    CHECK_EQ(VTA_ACC_WIDTH, 32);
    CHECK_EQ(VTA_INP_WIDTH, 8);

    int factor = VTA_ACC_WIDTH / VTA_INP_WIDTH;
    load_counter[0] += (op->x_size * op->y_size) * kElemBytes;
    if (skip_exec)
      return 0;
    DType* sram_ptr = data_ + op->sram_base;
    // int8_t* dram_ptr = static_cast<int8_t*>(dram->GetAddr(op->dram_base *
    // kElemBytes / factor));
    int8_t* dram_ptr = reinterpret_cast<int8_t*>(op->dram_base * kElemBytes / factor);
    auto& front = frontReq<DramReq>(dram_req_map[tag]);
    if (dram_req_map.empty() || front->addr != (uint64_t)dram_ptr) {
      auto incr = 0;
      for (uint32_t y = 0; y < op->y_size; ++y) {
        // dram one element is 1 bytes rather than 4 bytes
        incr += kElemBytes / factor * op->x_stride;
      }
      auto len = incr;
      if (len > 0){
        auto req = std::make_unique<DramReq>();
        req->addr = (uint64_t)dram_ptr;
        req->id = tag;
        req->len = incr;
        enqueueReq<DramReq>(dram_req_map[tag], std::move(req));
      }
      // return 1;
    }

    // Wait for Response
    {
      std::unique_lock lk(m_proc);
      while (front->acquired_len < front->len) {
        sim_blocked = true;
        cv.notify_one();
        cv.wait(lk, [] { return !sim_blocked; });
      }
    }

    dequeueReq<DramReq>(dram_req_map[tag]);

    uint64_t xtotal = op->x_size + op->x_pad_0 + op->x_pad_1;
    uint32_t ytotal = op->y_size + op->y_pad_0 + op->y_pad_1;
    uint64_t sram_end = op->sram_base + xtotal * ytotal;
    CHECK_LE(sram_end, kMaxNumElem);

    memset(sram_ptr, 0, kElemBytes * xtotal * op->y_pad_0);
    sram_ptr += xtotal * op->y_pad_0;

    dram_ptr = (int8_t*)read_buffer_map[tag]->getHead();
    for (uint32_t y = 0; y < op->y_size; ++y) {
      memset(sram_ptr, 0, kElemBytes * op->x_pad_0);
      sram_ptr += op->x_pad_0;

      int32_t* sram_ele_ptr = (int32_t*)sram_ptr;
      for (uint32_t x = 0; x < op->x_size * VTA_BATCH * VTA_BLOCK_OUT; ++x) {
        *(sram_ele_ptr + x) = (int32_t) * (dram_ptr + x);
        // this don't work, because read_buffer_map is uint8_t, and returning
        // int32_t
      }
      sram_ptr += op->x_size;

      memset(sram_ptr, 0, kElemBytes * op->x_pad_1);
      sram_ptr += op->x_pad_1;

      // dram one element is 1 bytes rather than 4 bytes
      dram_ptr += kElemBytes / factor * op->x_stride;
    }
    uint64_t incr = (uint8_t*)dram_ptr - read_buffer_map[tag]->getHead();
    assert(incr <= front->acquired_len);
    read_buffer_map[tag]->pop(front->acquired_len);
    memset(sram_ptr, 0, kElemBytes * xtotal * op->y_pad_1);
    */
    return 0;
  }

  // Execute the store instruction on this SRAM apply trucation.
  // This relies on the elements is 32 bits
  template <int target_bits>
  int TruncStore(const VTAMemInsn* op) {
    CHECK_EQ(op->x_pad_0, 0);
    CHECK_EQ(op->x_pad_1, 0);
    CHECK_EQ(op->y_pad_0, 0);
    CHECK_EQ(op->y_pad_1, 0);
    int target_width = (target_bits * kLane + 7) / 8;
    uint64_t req_addr = op->dram_base * target_width;
    BitPacker<kBits> src(data_ + op->sram_base);

    int cnt = 0;
    for (uint32_t y = 0; y < op->y_size; ++y) {
      for (uint32_t x = 0; x < op->x_size; ++x) {
        uint32_t sram_base = y * op->x_size + x;
        uint32_t dram_base = y * op->x_stride + x;

        void* buffer = calloc(kLane*target_bits/8, 1);
        BitPacker<target_bits> dst(buffer);
        #ifdef DEBUG_FUNC_SIM
          std::cerr << "Func-sim: store request " << req->addr << " " << req->len << "kLane" << kLane << std::endl;
        #endif
        for (int i = 0; i < kLane; ++i) {
          dst.SetSigned(i,
                        src.GetSigned(sram_base * kLane + i));
          cnt++;
        }
        write_sim_memory(req_addr+dram_base*kLane*target_bits/8, buffer, kLane*target_bits/8, (int)vta::CstStr::DMA_STORE);
      }
    }

    // std::cerr << "len mismatch? " << req->len << " " << cnt << std::endl;
    // assert(("len mismatch %d %d\n", req->len, cnt) && req->len == cnt);
    
    // std::cerr << "enq store request " << req->addr << " " << req->len << "
    // target_bits=" << target_bits << " target_width="<< target_width << "
    // KLane=" << kLane<< std::endl;

    // Produce Store request
    // auto& matcher = perf_req_map[STORE_ID];
    // matcher.Produce(std::move(req));

    return 0;
  }

 private:
  /*! \brief internal data content */
  DType* data_;
};

/*!
 * \brief Memory information of special memory region.
 *  Use MemoryInfo as its container type
 */
class Profiler {
 public:
  /*! \brief The memory load statistics */
  uint64_t inp_load_nbytes{0};
  /*! \brief The memory load statistics */
  uint64_t wgt_load_nbytes{0};
  /*! \brief The ACC memory load statistics */
  uint64_t acc_load_nbytes{0};
  /*! \brief The ACC memory load statistics */
  uint64_t uop_load_nbytes{0};
  /*! \brief The ACC memory load statistics */
  uint64_t out_store_nbytes{0};
  /*! \brief instr counter for gemm */
  uint64_t gemm_counter{0};
  /*! \brief instr counter for ALU ops */
  uint64_t alu_counter{0};
  /*! \brief set debug mode */
  int64_t debug_flag{0};
  /*! \brief clear the profiler */
  void Clear() {
    inp_load_nbytes = 0;
    wgt_load_nbytes = 0;
    acc_load_nbytes = 0;
    uop_load_nbytes = 0;
    out_store_nbytes = 0;
    gemm_counter = 0;
    alu_counter = 0;
  }
  /*! \return Whether we should skip execution. */
  bool SkipExec() const {
    return false;
    return (debug_flag & DebugFlagMask::kSkipExec) != 0;
  }

  std::string AsJSON() {
    std::ostringstream os;
    os << "{\n"
       << " \"inp_load_nbytes\":" << inp_load_nbytes << ",\n"
       << " \"wgt_load_nbytes\":" << wgt_load_nbytes << ",\n"
       << " \"acc_load_nbytes\":" << acc_load_nbytes << ",\n"
       << " \"uop_load_nbytes\":" << uop_load_nbytes << ",\n"
       << " \"out_store_nbytes\":" << out_store_nbytes << ",\n"
       << " \"gemm_counter\":" << gemm_counter << ",\n"
       << " \"alu_counter\":" << alu_counter << "\n"
       << "}\n";
    return os.str();
  }

  static Profiler* ThreadLocal() {
    static thread_local Profiler inst;
    return &inst;
  }
};

// Simulate device
// TODO(tqchen,thierry): queue based event driven simulation.
class Device {
 public:
  Device() {
    id_counter = 0;
    prof_ = Profiler::ThreadLocal();
  }

  int Run(vta_phy_addr_t insn_phy_addr, uint32_t insn_count,
          uint32_t wait_cycles, uint64_t ps_ts) {
    // Enqueue load request
    std::cout << "Func sim registered : really >" << std::endl;
   
    uint32_t max_insn = 128;
    uint32_t total_insn = insn_count;
    uint32_t ites = (total_insn / max_insn);
    uint32_t remain = (total_insn % max_insn);

    for (uint32_t i = 0; i < ites; ++i) {

      void* buffer = malloc(max_insn * sizeof(VTAGenericInsn));
      read_sim_memory(insn_phy_addr+max_insn*sizeof(VTAGenericInsn)*i, buffer, max_insn * sizeof(VTAGenericInsn), (int)vta::CstStr::DMA_LOAD_INSN);
      VTAGenericInsn* insn = static_cast<VTAGenericInsn*>(buffer);
      this->MakeInsns(max_insn, insn, ps_ts);

      uint32_t insn_holder = 0;
      // std::cout << "Funsim : Start Run insn" << std::endl;
      while (1) {
        VTAGenericInsn* insn = reinterpret_cast<VTAGenericInsn*>(buffer)+insn_holder;
        vta::sim::Device::Run_Insn(insn, reinterpret_cast<void*>(this));
        // std::cout << "Funsim Finished Instruction: "  << insn_holder << std::endl;
        insn_holder++;
        if (insn_holder == max_insn) {
          break;
        }
      }
    }

    if (remain > 0) {
      void* buffer = malloc(remain * sizeof(VTAGenericInsn));
      read_sim_memory(insn_phy_addr+max_insn*sizeof(VTAGenericInsn)*ites, buffer, remain * sizeof(VTAGenericInsn), (int)vta::CstStr::DMA_LOAD_INSN);
      VTAGenericInsn* insn = static_cast<VTAGenericInsn*>(buffer);
      this->MakeInsns(remain, insn, ps_ts);

      uint32_t insn_holder = 0;
      // std::cout << "Funsim : Start Run insn" << std::endl;
      while (1) {
        VTAGenericInsn* insn = reinterpret_cast<VTAGenericInsn*>(buffer)+insn_holder;
        vta::sim::Device::Run_Insn(insn, reinterpret_cast<void*>(this));
        // std::cout << "Funsim Finished Instruction: "  << insn_holder << std::endl;
        insn_holder++;
        if (insn_holder == remain) {
          break;
        }
      }
    }

   

    return 0;
  }

 private:

  void MakeInsns(int insn_count, const VTAGenericInsn* insn, uint64_t ts) {
    // Keep tabs on dependence queues
    // Converter
    union VTAInsn c;
    // Iterate over all instructions
    for (int i = 0; i < insn_count; ++i) {
      NEW_TOKEN(token_class_ostxyuullupppp, new_token);
      new_token->opcode = static_cast<int>(vta::CstStr::EMPTY);
      new_token->subopcode = static_cast<int>(vta::CstStr::EMPTY);
      new_token->tstype = static_cast<int>(vta::CstStr::EMPTY);
      new_token->ts = ts;
      pnumInsn.pushToken(new_token);

      c.generic = insn[i];
      if (c.mem.opcode == VTA_OPCODE_LOAD || c.mem.opcode == VTA_OPCODE_STORE) {
        if (c.mem.x_size == 0) {
          if (c.mem.opcode == VTA_OPCODE_STORE) {
            // //printf("NOP-STORE-STAGE\n");
            new_token->opcode = static_cast<int>(vta::CstStr::STORE);
            new_token->subopcode = static_cast<int>(vta::CstStr::SYNC);
          } else if (c.mem.memory_type == VTA_MEM_ID_ACC || c.mem.memory_type == VTA_MEM_ID_ACC_8BIT || c.mem.memory_type == VTA_MEM_ID_UOP) {
            // //printf("NOP-COMPUTE-STAGE\n");
            new_token->opcode = static_cast<int>(vta::CstStr::COMPUTE);
            new_token->subopcode = static_cast<int>(vta::CstStr::SYNC);
          } else {
            new_token->opcode = static_cast<int>(vta::CstStr::LOAD);
            new_token->subopcode = static_cast<int>(vta::CstStr::SYNC);
          }
          new_token->pop_prev =  static_cast<int>(c.mem.pop_prev_dep);
          new_token->pop_next =  static_cast<int>(c.mem.pop_next_dep);
          new_token->push_prev =  static_cast<int>(c.mem.push_prev_dep);
          new_token->push_next =  static_cast<int>(c.mem.push_next_dep);
          continue;
        }
        // Print instruction field information
        if (c.mem.opcode == VTA_OPCODE_LOAD) {
          // //printf("LOAD ");
          if (c.mem.memory_type == VTA_MEM_ID_UOP) {
            new_token->opcode = static_cast<int>(vta::CstStr::COMPUTE);
            new_token->subopcode = static_cast<int>(vta::CstStr::LOADUOP);
          }
          if (c.mem.memory_type == VTA_MEM_ID_WGT){
            new_token->opcode = static_cast<int>(vta::CstStr::LOAD);
            new_token->subopcode = static_cast<int>(vta::CstStr::LOAD);
            new_token->tstype = static_cast<int>(vta::CstStr::WGT);
            
          } 
          if (c.mem.memory_type == VTA_MEM_ID_INP){
            new_token->opcode = static_cast<int>(vta::CstStr::LOAD);
            new_token->subopcode = static_cast<int>(vta::CstStr::LOAD);
            new_token->tstype = static_cast<int>(vta::CstStr::INP);
            
          } 
          if (c.mem.memory_type == VTA_MEM_ID_ACC) {
            new_token->opcode = static_cast<int>(vta::CstStr::COMPUTE);
            new_token->subopcode = static_cast<int>(vta::CstStr::LOADACC);
          }
        }
        if (c.mem.opcode == VTA_OPCODE_STORE) {
          new_token->opcode = static_cast<int>(vta::CstStr::STORE);
          new_token->subopcode = static_cast<int>(vta::CstStr::STORE);

        }

        new_token->pop_prev =  static_cast<int>(c.mem.pop_prev_dep);
        new_token->pop_next =  static_cast<int>(c.mem.pop_next_dep);
        new_token->push_prev =  static_cast<int>(c.mem.push_prev_dep);
        new_token->push_next =  static_cast<int>(c.mem.push_next_dep);
        new_token->xsize =  static_cast<int>(c.mem.x_size);
        new_token->ysize =  static_cast<int>(c.mem.y_size);

        // c.mem.x_pad_0 = 0;
        // c.mem.x_pad_1 = 0;
        // c.mem.y_pad_0 = 0;
        // c.mem.y_pad_1 = 0;

      } else if (c.mem.opcode == VTA_OPCODE_GEMM) {
        // Print instruction field information
        new_token->opcode = static_cast<int>(vta::CstStr::COMPUTE);
        new_token->subopcode = static_cast<int>(vta::CstStr::GEMM);

        new_token->pop_prev =  static_cast<int>(c.mem.pop_prev_dep);
        new_token->pop_next =  static_cast<int>(c.mem.pop_next_dep);
        new_token->push_prev =  static_cast<int>(c.mem.push_prev_dep);
        new_token->push_next =  static_cast<int>(c.mem.push_next_dep);
        new_token->xsize =  static_cast<int>(c.mem.x_size);
        new_token->ysize =  static_cast<int>(c.mem.y_size);
        new_token->uop_begin = static_cast<int>(c.gemm.uop_bgn);
        new_token->uop_end = static_cast<int>(c.gemm.uop_end);
        new_token->lp_1 = static_cast<int>(c.gemm.iter_out);
        new_token->lp_0 = static_cast<int>(c.gemm.iter_in);

      } else if (c.mem.opcode == VTA_OPCODE_ALU) {
        
        new_token->opcode = static_cast<int>(vta::CstStr::COMPUTE);
        new_token->subopcode = static_cast<int>(vta::CstStr::ALU);
        
        new_token->pop_prev =  static_cast<int>(c.mem.pop_prev_dep);
        new_token->pop_next =  static_cast<int>(c.mem.pop_next_dep);
        new_token->push_prev =  static_cast<int>(c.mem.push_prev_dep);
        new_token->push_next =  static_cast<int>(c.mem.push_next_dep);
        new_token->xsize =  static_cast<int>(c.mem.x_size);
        new_token->ysize =  static_cast<int>(c.mem.y_size);
        new_token->uop_begin = static_cast<int>(c.alu.uop_bgn);
        new_token->uop_end = static_cast<int>(c.alu.uop_end);
        new_token->lp_1 = static_cast<int>(c.alu.iter_out);
        new_token->lp_0 = static_cast<int>(c.alu.iter_in);
        new_token->use_alu_imm = static_cast<int>(c.alu.use_imm);

      } else if (c.mem.opcode == VTA_OPCODE_FINISH) {
        new_token->opcode = static_cast<int>(vta::CstStr::LOAD);
        new_token->subopcode = static_cast<int>(vta::CstStr::SYNC);
        new_token->tstype = static_cast<int>(vta::CstStr::FINISH);
      }
    }
  }

  static int Run_Insn(const VTAGenericInsn* insn, void* dev) {
    Device* device = reinterpret_cast<Device*>(dev);
    const VTAMemInsn* mem = reinterpret_cast<const VTAMemInsn*>(insn);
    const VTAGemInsn* gem = reinterpret_cast<const VTAGemInsn*>(insn);
    const VTAAluInsn* alu = reinterpret_cast<const VTAAluInsn*>(insn);
    int ans = 0;
    switch (mem->opcode) {
      case VTA_OPCODE_LOAD:
      #ifdef DEBUG_FUNC_SIM
        std::cout << "opcode  " << "VTA OPCODE LOAD" << std::endl;
      #endif
        ans = device->RunLoad(mem);
        return ans;
        break;
      case VTA_OPCODE_STORE:
        #ifdef DEBUG_FUNC_SIM
        std::cout << "opcode  " << "VTA OPCODE STORE" << std::endl;
        #endif
        ans = device->RunStore(mem);
        return ans;
        break;
      case VTA_OPCODE_GEMM:
        #ifdef DEBUG_FUNC_SIM
        std::cout << "opcode  " << "VTA OPCODE GEMM" << std::endl;
        #endif
        ans = device->RunGEMM(gem);
        return ans;
        break;
      case VTA_OPCODE_ALU:
        #ifdef DEBUG_FUNC_SIM
        std::cout << "opcode  " << "VTA OPCODE ALU" << std::endl;
        #endif
        ans = device->RunALU(alu);
        return ans;
        break;
      case VTA_OPCODE_FINISH:
        ++(device->finish_counter_);
        #ifdef DEBUG_FUNC_SIM
        std::cerr << "opcode  " << "VTA OPCODE FINISH" << std::endl;
        #endif
        break;
      default: {
        // std::cerr << "Unknown op_code" << mem->opcode;
      }
    }
    return 0;
  }

 private:
  int RunLoad(const VTAMemInsn* op) {
    if (op->x_size == 0)
      return 0;
    if (op->memory_type == VTA_MEM_ID_INP) {
      #ifdef DEBUG_FUNC_SIM
      std::cout << "memory_type " << "VTA_MEM_ID_INP" << std::endl;
      #endif
      return inp_.Load(op, &(prof_->inp_load_nbytes), prof_->SkipExec(),
                       (int)vta::CstStr::DMA_LOAD_INP);
    } else if (op->memory_type == VTA_MEM_ID_WGT) {
      #ifdef DEBUG_FUNC_SIM
      std::cout << "memory_type " << "VTA_MEM_ID_WGT" << std::endl;
      #endif
      return wgt_.Load(op, &(prof_->wgt_load_nbytes), prof_->SkipExec(),
                       (int)vta::CstStr::DMA_LOAD_WGT);
    } else if (op->memory_type == VTA_MEM_ID_ACC) {
      #ifdef DEBUG_FUNC_SIM
      std::cout << "memory_type " << "VTA_MEM_ID_ACC" << std::endl;
      #endif
      return acc_.Load(op, &(prof_->acc_load_nbytes), prof_->SkipExec(),
                       (int)vta::CstStr::DMA_LOAD_ACC);
    } else if (op->memory_type == VTA_MEM_ID_UOP) {
      #ifdef DEBUG_FUNC_SIM
      std::cout << "memory_type " << "VTA_MEM_ID_UOP" << std::endl;
      #endif
      //  always load in uop, since uop is stateful
      //  subsequent non-debug mode exec can depend on it.
      return uop_.Load(op, &(prof_->uop_load_nbytes), false, (int)vta::CstStr::DMA_LOAD_UOP);
    } else if (op->memory_type == VTA_MEM_ID_ACC_8BIT) {
      // std::cout << "memory_type " << "VTA_MEM_ID_ACC_8BIT" << std::endl;
      return acc_.Load_int8(op, &(prof_->acc_load_nbytes), prof_->SkipExec(),
                            (int)vta::CstStr::DMA_LOAD_ACC);
    } else {
      std::cerr << "Unknown memory_type=" << op->memory_type;
      return 0;
    }
  }

  int RunStore(const VTAMemInsn* op) {
    if (op->x_size == 0)
      return 0;
    if (op->memory_type == VTA_MEM_ID_OUT) {
      prof_->out_store_nbytes += (op->x_size * op->y_size * VTA_BATCH *
                                  VTA_BLOCK_OUT * VTA_OUT_WIDTH / 8);
      if (!prof_->SkipExec()) {
        return acc_.TruncStore<VTA_OUT_WIDTH>(op);
      }
    } else {
      // std::cerr << "Store do not support memory_type=" << op->memory_type <<
      // std::endl;
      return 0;
    }
    return 1;
  }

  int RunGEMM(const VTAGemInsn* op) {
    // std::cerr << "GEMM params " << op->reset_reg << " " << " skip " << prof_->SkipExec() << " " << op->iter_out << " " << op->iter_in << " "
    //           << op->uop_bgn << " " << op->uop_end << std::endl;
    if (!op->reset_reg) {
      prof_->gemm_counter +=
          op->iter_out * op->iter_in * (op->uop_end - op->uop_bgn);
      if (prof_->SkipExec())
        return 0;
      // std::cerr << "GEMM params " << op->iter_out << " " << op->iter_in << " "
      //           << op->uop_bgn << " " << op->uop_end << std::endl;
      for (uint32_t y = 0; y < op->iter_out; ++y) {
        for (uint32_t x = 0; x < op->iter_in; ++x) {
          for (uint32_t uindex = op->uop_bgn; uindex < op->uop_end; ++uindex) {
            VTAUop* uop_ptr = static_cast<VTAUop*>(uop_.BeginPtr(uindex));
            // Read in memory indices
            uint32_t acc_idx = uop_ptr->dst_idx;
            uint32_t inp_idx = uop_ptr->src_idx;
            uint32_t wgt_idx = uop_ptr->wgt_idx;

            acc_idx += y * op->dst_factor_out + x * op->dst_factor_in;
            inp_idx += y * op->src_factor_out + x * op->src_factor_in;
            wgt_idx += y * op->wgt_factor_out + x * op->wgt_factor_in;
            BitPacker<VTA_ACC_WIDTH> acc(acc_.BeginPtr(acc_idx));
            BitPacker<VTA_INP_WIDTH> inp(inp_.BeginPtr(inp_idx));
            BitPacker<VTA_WGT_WIDTH> wgt(wgt_.BeginPtr(wgt_idx));

            // gemm loop
            for (uint32_t i = 0; i < VTA_BATCH; ++i) {
              for (uint32_t j = 0; j < VTA_BLOCK_OUT; ++j) {
                uint32_t acc_offset = i * VTA_BLOCK_OUT + j;
                int32_t sum = acc.GetSigned(acc_offset);
                for (uint32_t k = 0; k < VTA_BLOCK_IN; ++k) {
                  sum += inp.GetSigned(i * VTA_BLOCK_IN + k) *
                         wgt.GetSigned(j * VTA_BLOCK_IN + k);
                }
                acc.SetSigned(acc_offset, sum);
              }
            }
          }
        }
      }
    } else {
      if (prof_->SkipExec())
        return 0;
      // reset
      // std::cerr << "GEMM params " << op->iter_out << " " << op->iter_in << " "
      //           << op->uop_bgn << " " << op->uop_end << std::endl;

      for (uint32_t y = 0; y < op->iter_out; ++y) {
        for (uint32_t x = 0; x < op->iter_in; ++x) {
          for (uint32_t uindex = op->uop_bgn; uindex < op->uop_end; ++uindex) {
            VTAUop* uop_ptr = static_cast<VTAUop*>(uop_.BeginPtr(uindex));
            uint32_t acc_idx = uop_ptr->dst_idx;
            acc_idx += y * op->dst_factor_out + x * op->dst_factor_in;
            BitPacker<VTA_ACC_WIDTH> acc(acc_.BeginPtr(acc_idx));
            for (uint32_t i = 0; i < VTA_BATCH * VTA_BLOCK_OUT; ++i) {
              acc.SetSigned(i, 0);
            }
          }
        }
      }
    }
    return 0;
  }

  int RunALU(const VTAAluInsn* op) {
    if (op->use_imm) {
      return RunALU_<true>(op);
    } else {
      return RunALU_<false>(op);
    }
  }

  template <bool use_imm>
  int RunALU_(const VTAAluInsn* op) {
    switch (op->alu_opcode) {
      case VTA_ALU_OPCODE_ADD: {
        return RunALULoop<use_imm>(op,
                                   [](int32_t x, int32_t y) { return x + y; });
      }
      case VTA_ALU_OPCODE_MAX: {
        return RunALULoop<use_imm>(
            op, [](int32_t x, int32_t y) { return std::max(x, y); });
      }
      case VTA_ALU_OPCODE_MIN: {
        return RunALULoop<use_imm>(
            op, [](int32_t x, int32_t y) { return std::min(x, y); });
      }
      case VTA_ALU_OPCODE_SHR: {
        return RunALULoop<use_imm>(op, [](int32_t x, int32_t y) {
          if (y >= 0) {
            return x >> y;
          } else {
            return x << (-y);
          }
        });
      }
      case VTA_ALU_OPCODE_MUL: {
        return RunALULoop<use_imm>(op,
                                   [](int32_t x, int32_t y) { return x * y; });
      }
      default: {
        std::cerr << "Unknown ALU code " << op->alu_opcode;
        return 0;
      }
    }
  }

  template <bool use_imm, typename F>
  int RunALULoop(const VTAAluInsn* op, F func) {
    prof_->alu_counter +=
        op->iter_out * op->iter_in * (op->uop_end - op->uop_bgn);
    if (prof_->SkipExec())
      return 0;
    for (int y = 0; y < op->iter_out; ++y) {
      for (int x = 0; x < op->iter_in; ++x) {
        for (int k = op->uop_bgn; k < op->uop_end; ++k) {
          // Read micro op
          VTAUop* uop_ptr = static_cast<VTAUop*>(uop_.BeginPtr(k));
          uint32_t dst_index = uop_ptr->dst_idx;
          uint32_t src_index = uop_ptr->src_idx;
          dst_index += y * op->dst_factor_out + x * op->dst_factor_in;
          src_index += y * op->src_factor_out + x * op->src_factor_in;
          BitPacker<VTA_ACC_WIDTH> dst(acc_.BeginPtr(dst_index));
          BitPacker<VTA_ACC_WIDTH> src(acc_.BeginPtr(src_index));
          for (int k = 0; k < VTA_BATCH * VTA_BLOCK_OUT; ++k) {
            if (use_imm) {
              dst.SetSigned(k, func(dst.GetSigned(k), op->imm));
            } else {
              dst.SetSigned(k, func(dst.GetSigned(k), src.GetSigned(k)));
            }
          }
        }
      }
    }
    return 0;
  }
  // the finish counter
  int finish_counter_{0};
  // Prof_
  Profiler* prof_;
  // The DRAM interface
  SRAM<VTA_INP_WIDTH, VTA_BATCH * VTA_BLOCK_IN, VTA_INP_BUFF_DEPTH> inp_;
  SRAM<VTA_WGT_WIDTH, VTA_BLOCK_IN * VTA_BLOCK_OUT, VTA_WGT_BUFF_DEPTH> wgt_;
  SRAM<VTA_ACC_WIDTH, VTA_BATCH * VTA_BLOCK_OUT, VTA_ACC_BUFF_DEPTH> acc_;
  SRAM<VTA_UOP_WIDTH, 1, VTA_UOP_BUFF_DEPTH> uop_;
};

}  // namespace sim
}  // namespace vta

VTADeviceHandle VTADeviceAlloc() {
  return new vta::sim::Device();
}

void VTADeviceFree(VTADeviceHandle handle) {
  delete static_cast<vta::sim::Device*>(handle);
}

int VTADeviceRun(VTADeviceHandle handle, vta_phy_addr_t insn_phy_addr,
                 uint32_t insn_count, uint32_t wait_cycles, uint64_t ps_ts) {
  return static_cast<vta::sim::Device*>(handle)->Run(insn_phy_addr, insn_count,
                                                     wait_cycles, ps_ts);
}
