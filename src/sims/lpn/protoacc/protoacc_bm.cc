#include "include/protoacc_bm.hh"

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

#include <simbricks/pciebm/pciebm.hh>

#include "include/lpn_req_map.hh"
#include "sims/lpn/protoacc/include/protoacc_regs.hh"
#include "sims/lpn/lpn_common/place_transition.hh"
#include "sims/lpn/lpn_common/lpn_sim.hh"
#include "sims/lpn/protoacc/include/lpn_req_map.hh"
#include "sims/lpn/protoacc/include/driver.hh"
#include "sims/lpn/protoacc/perf_sim/sim.hh"


// if you are waiting for the zero-cost-dma, 
// and you don't have events to schedule
// you can't send sync messages yet

#define MASK5 0b11111
#define MASK6 0b111111

#define Protoacc_DEBUG 0

// #define Protoacc_DEBUG_DMA

uint64_t in_flight_write = 0;
uint64_t in_flight_read = 0;

PACBm* protoacc_bm_;

namespace{

PACBm protoacc_sim{};

double start_time;

void sigint_handler(int dummy) {
  protoacc_sim.SIGINTHandler();
}

void sigusr1_handler(int dummy) {
  protoacc_sim.SIGUSR1Handler();
}

void sigusr2_handler(int dummy) {
  protoacc_sim.SIGUSR2Handler();
}

}  // namesProtoacce

void PACBm::SetupIntro(struct SimbricksProtoPcieDevIntro &dev_intro) {

  std::cout << "PACBm::SetupIntro" << std::endl;
  
  dev_intro.pci_vendor_id = 0xdead;
  dev_intro.pci_device_id = 0xbeef;
  dev_intro.pci_class = 0x40;
  dev_intro.pci_subclass = 0x00;
  dev_intro.pci_revision = 0x00;

  // request one BAR
  static_assert(sizeof(PACRegs) <= 4096, "Registers don't fit BAR");
  dev_intro.bars[0].len = 4096;
  dev_intro.bars[0].flags = 0;

  // setup LPN initial state
  // auto ids = std::vector<int>{LOAD_INSN, LOAD_INP_ID, LOAD_WGT_ID, LOAD_ACC_ID, LOAD_UOP_ID, STORE_ID};
  setupReqQueues(ids);
  lpn_init();
  protoacc_bm_ = &protoacc_sim;
}

void PACBm::RegRead(uint8_t bar, uint64_t addr, void *dest,
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

static uint64_t started_task = 0;
void PACBm::RegWrite(uint8_t bar, uint64_t addr, const void *src,
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

  // link to the address of the register
  std:: cerr << "Ctrl value: " << Registers_.ctrl << std::hex << "\n";
  uint32_t update_output_ptrs = (Registers_.ctrl >> 7 & 0x1) == 0x1;
  if(update_output_ptrs){
    Registers_.ctrl = 0;
    printf("Update output pointers\n");
    lpn_setup_output_addr(
      (uint64_t)Registers_.string_ptr_region_ptr_as_int_h << 32 | Registers_.string_ptr_region_ptr_as_int_l,
      (uint64_t)Registers_.stringalloc_region_ptr_as_int_tail_h << 32 | Registers_.stringalloc_region_ptr_as_int_tail_l
      );
  }

  uint32_t start = Registers_.ctrl  == 1;
  if (start){
    Registers_.ctrl = 0;
    started_task ++;
    std:: cerr << "start LPN : " << Registers_.ctrl << std::hex << " total task " << started_task << "\n";
    lpn_start(
            (uint64_t)Registers_.descriptor_table_addr_h << 32 | Registers_.descriptor_table_addr_l, 
            (uint64_t)Registers_.src_base_addr_h << 32 | Registers_.src_base_addr_l, 
              TimePs());

    for (auto &kv : io_req_map) {
      std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
    }

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
void PACBm::DmaComplete(std::unique_ptr<pciebm::DMAOp> dma_op) {

  uint64_t ts_now = TimePs();
  UpdateClk(t_list, T_SIZE, ts_now);
  // handle response to DMA read request
  if (!dma_op->write) {
    in_flight_read--;
    putData(dma_op->dma_addr, dma_op->len, dma_op->tag, dma_op->write, ts_now, dma_op->data);
    #ifdef Protoacc_DEBUG_DMA
      std::cerr << TimePs()/1000 << " DMA Complete: " << dma_op->tag << " " << dma_op->dma_addr << " " << dma_op->len << std::endl;
    #endif
  }
  // handle response to DMA write request
  else {
    putData(dma_op->dma_addr, dma_op->len, dma_op->tag, dma_op->write, ts_now, dma_op->data);
    in_flight_write--;
    #ifdef Protoacc_DEBUG_DMA
      std::cerr << TimePs()/1000 << " DMA Write Complete: " << dma_op->tag << " " << dma_op->dma_addr << " " << dma_op->len << std::endl;
    #endif
    // Process Write
    // lpn_req->acquired_len += dma_op->len;
  }

  // Run LPN to process received memory
  uint64_t next_ts = NextCommitTime(t_list, T_SIZE); 
  
  // Check for end condition
  if (in_flight_write == 0 && in_flight_read == 0 && lpn_finished() && next_ts == lpn::LARGE) {
    std::cerr << "DMAcomplete: ProtoaccDeviceRun finished " << std::endl;
    // lpn_end();
    ClearReqQueues(ids);

    // Registers_.ctrl = 0x2;
    Registers_.completed_msg = started_task;

    // interupt indicate the protoacc is done
    MsiXIssue(0);
    
    // TransitionCountLog(t_list, T_SIZE);
    return ;
  }

  // Schedule next event
  #if Protoacc_DEBUG
      std::cerr << "next_ts=" << next_ts <<  " TimePs=" << TimePs() << " lpnLarge=" << lpn::LARGE << "\n";
  #endif
      // assert(next_ts >= TimePs() &&
      //        "PACBm::DmaComplete: Cannot schedule event for past timestamp");
      if(next_ts < TimePs()){
        std::cerr << "Warning: next_ts is in the past: " << next_ts << " < " << TimePs() << "; force update \n";
        next_ts = TimePs();
      }
      auto next_scheduled = EventNext();
      if (next_ts != lpn::LARGE &&
          (!next_scheduled || next_scheduled.value() > next_ts)) {
  #if Protoacc_DEBUG
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

void PACBm::FastForward(){
  UpdateClk(t_list, T_SIZE, TimePs());
}

void PACBm::ExecuteEvent(std::unique_ptr<pciebm::TimedEvent> evt) {
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

#if Protoacc_DEBUG
  std::cerr << "lpn exec: evt time=" << evt->time << " TimePs=" << TimePs()
            << " next_ts=" << next_ts <<  " lpnLarge=" << lpn::LARGE << "\n";
#endif
  // only schedule an event if one doesn't exist yet
  // assert(next_ts >= TimePs() &&
      // "PACBm::ExecuteEvent: Cannot schedule event for past timestamp");
  
  if(next_ts < TimePs()){
    std::cerr << "Warning: next_ts is in the past: " << next_ts << " < " << TimePs() << "; force update \n";
    next_ts = TimePs();
  }

  auto next_scheduled = EventNext();

  if (next_ts != lpn::LARGE &&
      (!next_scheduled || next_scheduled.value() > next_ts)) {
#if Protoacc_DEBUG
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
      while(total_bytes > 0){
        auto bytes_to_req = std::min<uint64_t>(total_bytes, DMA_BLOCK_SIZE);
        if (req->rw == READ_REQ) {
          in_flight_read++;
          auto dma_op = std::make_unique<PACDmaReadOp<DMA_BLOCK_SIZE>>(req->addr + sent_bytes, bytes_to_req, req->tag);
          #ifdef Protoacc_DEBUG_DMA
            std::cerr << "Issue DMA Read: " << req->tag << " " << req->addr + sent_bytes << " " << bytes_to_req << std::endl;
          #endif
          IssueDma(std::move(dma_op));
        } else {
          // reset the len to record for completion
          req->acquired_len = 0;
          auto dma_op = std::make_unique<PACDmaWriteOp>(req->addr + sent_bytes, bytes_to_req, req->tag);
          std::memcpy(dma_op->buffer, req->buffer, bytes_to_req);
          in_flight_write++;
          #ifdef Protoacc_DEBUG_DMA
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

  if (in_flight_write == 0 && in_flight_read == 0 && lpn_finished() && next_ts == lpn::LARGE) {
      std::cerr << "ProtoaccDeviceRun finished " << std::endl;
      
      for (auto &kv : io_req_map) {
        std::cerr << "io_req_map[" << kv.first << "].size() = " << kv.second.size() << "\n";
      }

      ClearReqQueues(ids);
      // lpn_end();

      // Registers_.ctrl = 0x2;
      Registers_.completed_msg = started_task;
      // interupt indicate the protoacc is done
      MsiXIssue(0);
      // TransitionCountLog(t_list, T_SIZE);
      return;
  }
}

void PACBm::DevctrlUpdate(
    struct SimbricksProtoPcieH2DDevctrl &devctrl) {
  // ignore this for now
  std::cerr << "warning: ignoring SimBricks DevCtrl message with flags "
            << devctrl.flags << "\n";
}


int main(int argc, char *argv[]) {
  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);
  signal(SIGUSR2, sigusr2_handler);
  if (!protoacc_sim.ParseArgs(argc, argv)) {
    return EXIT_FAILURE;
  }
  return protoacc_sim.RunMain();
}
