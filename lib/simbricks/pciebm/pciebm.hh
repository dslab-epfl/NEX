/*
 * Copyright 2024 Max Planck Institute for Software Systems, and
 * National University of Singapore
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SIMBRICKS_PCIEBM_PCIEBM_H_
#define SIMBRICKS_PCIEBM_PCIEBM_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>

#include <simbricks/base/cxxatomicfix.h>
extern "C" {
#include "simbricks/pcie/if.h"
#include "simbricks/mem/if.h"
}

namespace pciebm {

struct DMAOp {
  uint32_t tag;
  bool write;
  uint64_t dma_addr;
  size_t len;
  uint8_t *data;
};

struct TimedEvent {
  uint64_t time;
  int priority;
};

/* This is required for the priority queue of references to `TimedEvent`*/
struct TimedEventPtrGreater {
  bool operator()(const std::unique_ptr<TimedEvent> &lhs,
                  const std::unique_ptr<TimedEvent> &rhs) const {
    return lhs->time > rhs->time ||
           (lhs->time == rhs->time && lhs->priority > rhs->priority);
  }
};

/* This is an abstract base for PCIe device simulators that implement a
 * behavioral model. The idea is to inherit from this class and implement the
 * virtual methods, which are mostly callbacks for important PCIe events.
 * Furthermore, the remaining protected methods expose an API to invoke PCIe
 * requests. The private methods take care of implementing the low-level
 * SimBricks related details. */
class PcieBM {
 protected:
  /**
   * These are callback functions that need to be implemented by the behavioral
   * model
   */

  /* Initialize device specific parameters (pci dev/vendor id, BARs etc. in
   * `di`.*/
  virtual void SetupIntro(struct SimbricksProtoPcieDevIntro &devintro) = 0;

  /* Invoked when receiving a register read request `bar`:`addr` of length
   * `len`. Store the result in `dest`. */
  virtual void RegRead(uint8_t bar, uint64_t addr, void *dest, size_t len) = 0;

  /* Invoked when receiving a register read request `bar`:`addr` of length
   * `len`, with the data in `src`. */
  virtual void RegWrite(uint8_t bar, uint64_t addr, const void *src,
                        size_t len) = 0;

  /* The previously issued DMA operation `op` has been completed. */
  virtual void DmaComplete(std::unique_ptr<DMAOp> dma_op) = 0;

  /* Callback for executing the previously scheduled event `evt`. */
  virtual void ExecuteEvent(std::unique_ptr<TimedEvent> evt) = 0;

  /* Callback for a device control update request. */
  virtual void DevctrlUpdate(struct SimbricksProtoPcieH2DDevctrl &devctrl) = 0;

  virtual void FastForward() = 0;

  /**
   * The following functions form the API exposed to the behavioral model for
   * invoking PCIe requests and scheduling events.
   */

  /* Issue a DMA PCIe request with the details in `dma_op`. */
  void IssueDma(std::unique_ptr<DMAOp> dma_op);

  /* Issue an MSI interrupt over PCIe. */
  void MsiIssue(uint8_t vec);

  /* Issue an MSI-X interrupt over PCIe. */
  void MsiXIssue(uint8_t vec);

  /* Issue a legacy interrupt over PCIe. */
  void IntXIssue(bool level);

  /* Returns the current timestamp in picoseconds. */
  uint64_t TimePs() const;

  /* Schedule an event to be executed in the future. */
  void EventSchedule(std::unique_ptr<TimedEvent> evt);

  /* Returns the timestamp of the earliest event that's scheduled to be
   * executed. If no scheduled event exists, returns an empty std::optional */
  std::optional<uint64_t> EventNext();

  

 private:
  uint64_t main_time_ = 0;
  uint32_t dma_read_max_pending_;
  uint32_t dma_write_max_pending_;
  std::priority_queue<std::unique_ptr<TimedEvent>,
                      std::vector<std::unique_ptr<TimedEvent>>,
                      TimedEventPtrGreater>
      events_{};
  std::queue<std::unique_ptr<DMAOp>> dma_read_queue_{};
  std::queue<std::unique_ptr<DMAOp>> dma_write_queue_{};
  std::unordered_map<uintptr_t, std::unique_ptr<DMAOp>> dma_read_pending_{};
  std::unordered_map<uintptr_t, std::unique_ptr<DMAOp>> dma_write_pending_{};

  struct SimbricksBaseIfParams pcieParams_;
  struct SimbricksBaseIfParams memParams_;
  const char *pcieShmPath_ = nullptr;
  const char *memShmPath_ = nullptr;
  struct SimbricksPcieIf pcieif_;
  struct SimbricksMemIf memif_;
  struct SimbricksProtoPcieDevIntro dintro_;
  struct SimbricksProtoMemHostIntro mintro_;

  /* for signal handlers */
  volatile bool exiting_ = false;
  volatile bool stat_flag_ = false;

  uint64_t h2d_poll_total_ = 0;
  uint64_t h2d_poll_suc_ = 0;
  uint64_t h2d_poll_sync_ = 0;
  /* counted from signal USR2 */
  uint64_t s_h2d_poll_total_ = 0;
  uint64_t s_h2d_poll_suc_ = 0;
  uint64_t s_h2d_poll_sync_ = 0;

  uint64_t n2d_poll_total_ = 0;
  uint64_t n2d_poll_suc_ = 0;
  uint64_t n2d_poll_sync_ = 0;
  /* counted from signal USR2 */
  uint64_t s_n2d_poll_total_ = 0;
  uint64_t s_n2d_poll_suc_ = 0;
  uint64_t s_n2d_poll_sync_ = 0;

  volatile union SimbricksProtoPcieD2H *D2HAlloc();

  void H2DRead(volatile struct SimbricksProtoPcieH2DRead *read);
  void H2DWrite(volatile struct SimbricksProtoPcieH2DWrite *write, bool posted);
  void H2DReadcomp(volatile struct SimbricksProtoPcieH2DReadcomp *readcomp);
  void H2DWritecomp(volatile struct SimbricksProtoPcieH2DWritecomp *writecomp);
  void H2DDevctrl(volatile struct SimbricksProtoPcieH2DDevctrl *devctrl);
  bool PollH2D();
  void H2DFastforward(volatile struct SimbricksProtoPcieH2DForceupadte &msg);

  bool EventTrigger();

  void DmaDo(std::unique_ptr<DMAOp> dma_op);
  void DmaTrigger();

  void YieldPoll();
  bool SimBricksIfsInit();
  bool MemIfInit();

 public:
  PcieBM(uint32_t dma_max_pending) : dma_read_max_pending_(dma_max_pending), dma_write_max_pending_(dma_max_pending) {}

  /** Parse command line arguments. */
  bool ParseArgs(int argc, char *argv[]);

  /** Run the simulation */
  int RunMain();
  int MemRunMain();

  /* This handler should be invoked when receiving a SIGINT signal. */
  void SIGINTHandler();
  /* This handler should be invoked when receiving a SIGUSR1 signal. */
  void SIGUSR1Handler();
  /* This handler should be invoked when receiving a SIGUSR2 signal. */
  void SIGUSR2Handler();

  std::unique_ptr<DMAOp> ZeroCostBlockingDma(std::unique_ptr<DMAOp> dma_op);
};

}  // namespace pciebm
#endif  // SIMBRICKS_PCIEBM_PCIEBM_H_