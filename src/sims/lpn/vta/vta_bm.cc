#include "include/vta_bm.hh"

#include <bits/stdint-uintn.h>
#include <bits/types/siginfo_t.h>
#include <signal.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <sys/time.h>
#include <sys/types.h>

#include <simbricks/pciebm/pciebm.hh>

#include "include/lpn_req_map.hh"
#include "sims/lpn/vta/include/vta_regs.hh"
#include "sims/lpn/lpn_common/place_transition.hh"
#include "sims/lpn/lpn_common/lpn_sim.hh"
#include "sims/lpn/vta/include/lpn_req_map.hh"
#include "sims/lpn/vta/include/driver.hh"
#include "sims/lpn/vta/perf_sim/sim.hh"
#include "sims/lpn/vta/include/vta/driver.h"


// if you are waiting for the zero-cost-dma, 
// and you don't have events to schedule
// you can't send sync messages yet

#define MASK5 0b11111
#define MASK6 0b111111

#define VTA_DEBUG 0

// #define VTA_DEBUG_DMA

uint64_t in_flight_write = 0;
uint64_t in_flight_read = 0;

VTABm* vta_bm_;

namespace{

VTABm vta_sim{};
VTADeviceHandle vta_func_device;

void sigint_handler(int dummy) {
  vta_sim.SIGINTHandler();
}

void sigusr1_handler(int dummy) {
  vta_sim.SIGUSR1Handler();
}

void sigusr2_handler(int dummy) {
  vta_sim.SIGUSR2Handler();
}

}  // namespace VTA

int finish_condition(uint64_t next_ts){
  return in_flight_write == 0 &&
         in_flight_read == 0 && 
         dma_read_requests.empty() &&
         dma_write_requests.empty() &&
         dma_read_resp.empty() &&
         dma_write_resp.empty() &&
         next_ts == lpn::LARGE &&
         lpn_finished();
}


void VTABm::SetupIntro(struct SimbricksProtoPcieDevIntro &dev_intro) {

  std::cout << "VTABm::SetupIntro" << std::endl;
  
  dev_intro.pci_vendor_id = 0xdead;
  dev_intro.pci_device_id = 0xbeef;
  dev_intro.pci_class = 0x40;
  dev_intro.pci_subclass = 0x00;
  dev_intro.pci_revision = 0x00;

  // request one BAR
  static_assert(sizeof(VTARegs) <= 4096, "Registers don't fit BAR");
  dev_intro.bars[0].len = 4096;
  dev_intro.bars[0].flags = 0;

  // setup LPN initial state
  // auto ids = std::vector<int>{LOAD_INSN, LOAD_INP_ID, LOAD_WGT_ID, LOAD_ACC_ID, LOAD_UOP_ID, STORE_ID};
  setupReqQueues(ids);
  lpn_init();
  vta_bm_ = &vta_sim;
}

void VTABm::RegRead(uint8_t bar, uint64_t addr, void *dest,
                            size_t len) {
  if (bar != 0) {
    std::cerr << "error: register read from unmapped BAR " << bar << "\n";
    return;
  }
  if (addr + len > sizeof(Registers_)) {
    std::cerr << "error: register read is outside bounds offset=" << addr
              << " len=" << len << "\n";
    return;
  }
  
  std::memcpy(dest, reinterpret_cast<uint8_t *>(&Registers_) + addr, len);
  std::cerr << "Register read offset=" << addr
            << " len=" << len << " value=" << (*((uint32_t*)(dest))) <<"\n";
}

void VTABm::RegWrite(uint8_t bar, uint64_t addr, const void *src,
                             size_t len) {
  
  if (bar != 0) {
    std::cerr << "error: register write to unmapped BAR " << bar << "\n";
    return;
  }
  if (addr + len > sizeof(Registers_)) {
    std::cerr << "error: register write is outside bounds offset=" << addr
              << " len=" << len << "\n";
    return;
  }

  std::cerr << "Register write offset=" << addr
              << " len=" << len << " value=" << (*((uint32_t*)(src))) <<"\n";
  std::memcpy(reinterpret_cast<uint8_t *>(&Registers_) + addr, src, len);

  // if (addr == 20 && (Registers_._0x14 & 0x1) == 0x1) {
  //   std::cerr << "Resetting vtabm and lpn" << std::endl;
  //   lpn_reset();
  //   std::memset(reinterpret_cast<uint8_t *>(&Registers_), 0, 36);
  //   return;
  // }
  uint32_t start = Registers_.ctrl  == 1;
  if (start){
    
    Registers_.ctrl = 0;

    uint64_t insn_phy_addr = Registers_.insn_phy_addr_hh; 
    insn_phy_addr = insn_phy_addr << 32 | Registers_.insn_phy_addr_lh;
    uint32_t insn_count = Registers_.insn_count;
    std::cout << "insn_phy_addr: " << insn_phy_addr << " insn_count: " << insn_count << std::endl;
    vta_func_device = VTADeviceAlloc();
    VTADeviceRun(vta_func_device, insn_phy_addr, insn_count, 1000000, TimePs());

    std::cerr << "LAUNCHING LPN with insns: " << insn_count << std::endl;
    lpn_start(insn_count, TimePs());

    // PlaceTokensLog(t_list, T_SIZE);

    for (auto &kv : io_req_map) {
      std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
    }

    TransitionResetCount(t_list, T_SIZE);

    // Start simulating the LPN immediately
    auto evt = std::make_unique<pciebm::TimedEvent>();
    evt->time = TimePs();
    evt->priority = 0;
    EventSchedule(std::move(evt));
  }
}

// Algo For Read
// 1. Process Read, put into lpn_req buffer
// 2. Run LPN to produce mem requests
// 3. Notify func sim
// 4. Issue new DMA ops 
void VTABm::DmaComplete(std::unique_ptr<pciebm::DMAOp> dma_op) {

  UpdateClk(t_list, T_SIZE, TimePs());
  // handle response to DMA read request
  if (!dma_op->write) {
    in_flight_read--;
    putData(dma_op->dma_addr, dma_op->len, dma_op->tag, dma_op->write, TimePs(), dma_op->data);
    #ifdef VTA_DEBUG_DMA
      std::cerr << TimePs()/1000 << " DMA Complete: " << dma_op->tag << " " << dma_op->dma_addr << " " << dma_op->len << std::endl;
    #endif
  }
  // handle response to DMA write request
  else {
    putData(dma_op->dma_addr, dma_op->len, dma_op->tag, dma_op->write, TimePs(), dma_op->data);
    in_flight_write--;
    #ifdef VTA_DEBUG_DMA
      std::cerr << TimePs()/1000 << " DMA Write Complete: " << dma_op->tag << " " << dma_op->dma_addr << " " << dma_op->len << std::endl;
    #endif
    // Process Write
    // lpn_req->acquired_len += dma_op->len;
  }

  // Run LPN to process received memory
  uint64_t next_ts = NextCommitTime(t_list, T_SIZE); 
  
  // Check for end condition
  if (finish_condition(next_ts)) {
    std::cerr << "DMAcomplete: VTA finished " << std::endl;
    lpn_clear();
    ClearReqQueues(ids);
    

    Registers_.ctrl = 0x2;
    // MsiXIssue(0);
    // TransitionCountLog(t_list, T_SIZE);
    return ;
  }

  // Schedule next event
  #if VTA_DEBUG
      std::cerr << "next_ts=" << next_ts <<  " TimePs=" << TimePs() << " lpnLarge=" << lpn::LARGE << "\n";
  #endif
      assert(next_ts >= TimePs() &&
             "VTABm::DmaComplete: Cannot schedule event for past timestamp");
      auto next_scheduled = EventNext();
      if (next_ts != lpn::LARGE &&
          (!next_scheduled || next_scheduled.value() > next_ts)) {
  #if VTA_DEBUG
        std::cerr << "schedule next at = " << next_ts << "\n";
  #endif
        auto evt = std::make_unique<pciebm::TimedEvent>();
        evt->time = next_ts;
        EventSchedule(std::move(evt));
      }
}


void from_list_to_io_map(){
    while(!dma_read_requests.empty()){
        token_class_iasbrr* token = dma_read_requests.front();
        dma_read_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        if(io_req_map[tag].empty()){
            // printf("@dma_read_finish_directly port %d tag %d\n", token->ref, token->id);
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            dma_read_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        assert(req != nullptr);
        req->extra_ptr = static_cast<void*>(token);
        assert(req->tag == tag);
        // printf("@dma_read_start id %d; port %d tag %d\n", req->id, token->ref, token->id);
        io_send_req_map[tag].push_back(std::move(req));
    }

    while(!dma_write_requests.empty()){
        auto token = dma_write_requests.front();
        dma_write_requests.pop_front();
        auto tag = token->id;
        // id is the tag in io_req_map
        // ref is the port number
        if(io_req_map[tag].empty()){
            // printf("@dma_write_finish_directly port %d tag %d\n", token->ref, token->id);
            // no matched request, put response directly
            // no match because what lpn generates might not be the same as what the functional simulator expects
            dma_write_resp.push_back(token);
            continue;
        }
        auto req = dequeueReq(io_req_map[tag]);
        req->extra_ptr = static_cast<void*>(token);
        // printf("@dma_write_start id %d; port %d tag %d\n", req->id, token->ref, token->id);
        io_send_req_map[tag].push_back(std::move(req));
    }
}

void VTABm::ExecuteEvent(std::unique_ptr<pciebm::TimedEvent> evt) {
  // commit all transitions who can commit at evt.time
  // alternatively, commit transitions one by one.
  // UpdateClk(TimePs());‘
  uint64_t next_ts = lpn::LARGE;
  while(1){
    CommitAtTime(t_list, T_SIZE, evt->time);
    // TransitionCountLog(t_list, T_SIZE);
    next_ts = NextCommitTime(t_list, T_SIZE);
    if (next_ts > evt->time) break;
  }

  // check the new dma events
  from_list_to_io_map();
  next_ts = NextCommitTime(t_list, T_SIZE);

#if VTA_DEBUG
  std::cerr << "lpn exec: evt time=" << evt->time << " TimePs=" << TimePs()
            << " next_ts=" << next_ts <<  " lpnLarge=" << lpn::LARGE << "\n";
#endif
  // only schedule an event if one doesn't exist yet
  assert(next_ts >= TimePs() &&
      "VTABm::ExecuteEvent: Cannot schedule event for past timestamp");
  auto next_scheduled = EventNext();

  if (next_ts != lpn::LARGE &&
      (!next_scheduled || next_scheduled.value() > next_ts)) {
#if VTA_DEBUG
    std::cerr << "schedule next at = " << next_ts << "\n";
#endif
    evt->time = next_ts;
    evt->priority = 0;
    EventSchedule(std::move(evt));
  }

  // Issue requests enqueued by IOGen
  for (auto &kv : io_send_req_map) {
    if (kv.second.empty()) continue;
    while(!kv.second.empty()){
      auto req = dequeueReq(kv.second);
      auto total_bytes = req->len;
      auto sent_bytes = 0;
      // reset the len to record for completion
      req->acquired_len = 0;
      while(total_bytes > 0){
        auto bytes_to_req = std::min<uint64_t>(total_bytes, DMA_BLOCK_SIZE);
        if (req->rw == READ_REQ) {
          in_flight_read++;
          auto dma_op = std::make_unique<VTADmaReadOp<DMA_BLOCK_SIZE>>(req->addr + sent_bytes, bytes_to_req, req->tag);
          #ifdef VTA_DEBUG_DMA
            std::cerr << "Issue DMA Read: " << req->tag << " " << req->addr + sent_bytes << " " << bytes_to_req << std::endl;
          #endif
          IssueDma(std::move(dma_op));
        } else {
          auto dma_op = std::make_unique<VTADmaWriteOp>(req->addr + sent_bytes, bytes_to_req, req->tag);
          std::memcpy(dma_op->buffer, req->buffer, bytes_to_req);
          in_flight_write++;
          #ifdef VTA_DEBUG_DMA
            std::cerr << "Issue DMA Write: " << req->tag << " " << req->addr + sent_bytes << " " << bytes_to_req << std::endl;
          #endif
          IssueDma(std::move(dma_op));
        }
        total_bytes -= bytes_to_req;
        sent_bytes += bytes_to_req;
      }
      io_pending_req_map[kv.first].push_back(std::move(req));
    }
  }

  if (finish_condition(next_ts)) {
      std::cerr << "VTA Run finished " << std::endl;
      
      // TransitionCountLog(t_list, T_SIZE);
      PlaceTokensLog(t_list, T_SIZE);

      for (auto &kv : io_pending_req_map) {
        std::cerr << "io_pending_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
      }

      for (auto &kv : io_req_map) {
        std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
        assert(kv.second.size() == 0);
      }

      lpn_clear();
      ClearReqQueues(ids);
      Registers_.ctrl = 0x2;
      //MsiXIssue(0);
      // TransitionCountLog(t_list, T_SIZE);
      return;
  }
}

void VTABm::DevctrlUpdate(
    struct SimbricksProtoPcieH2DDevctrl &devctrl) {
  // ignore this for now
  std::cerr << "warning: ignoring SimBricks DevCtrl message with flags "
            << devctrl.flags << "\n";
}

void VTABm::FastForward(){
  UpdateClk(t_list, T_SIZE, TimePs());
}

int main(int argc, char *argv[]) {
  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);
  signal(SIGUSR2, sigusr2_handler);
  if (!vta_sim.ParseArgs(argc, argv)) {
    return EXIT_FAILURE;
  }
  return vta_sim.RunMain();
}
