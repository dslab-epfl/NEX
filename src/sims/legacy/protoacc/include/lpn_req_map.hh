#ifndef __LPN_REQ_MAP_HH
#define __LPN_REQ_MAP_HH

#include <assert.h>
#include <bits/stdint-uintn.h>

#include <condition_variable>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <deque>
#include <vector>

// TODO Rename MATCH Interface

typedef struct MemReq {
  uint32_t id;
  int tag;
  uint64_t addr;
  uint32_t len;
  uint32_t acquired_len;
  bool rw;
  uint64_t issued_ts;
  uint64_t complete_ts;
  void* extra_ptr;
  // 0 is not ready for issue
  // 1 is ready for issue
  // 2 is issued
  // 3 is completes
  int issue = 0;
} MemReq;

#define READ_REQ 0
#define WRITE_REQ 1

typedef struct DramReq : MemReq {
} DramReq;

extern std::map<int, std::deque<std::unique_ptr<MemReq>>> io_req_map;
extern std::map<int, std::deque<std::unique_ptr<MemReq>>> io_pending_map;

void setupReqQueues(const std::vector<int>& ids);
void ClearReqQueues(const std::vector<int>& ids);

std::unique_ptr<MemReq>& frontReq(std::deque<std::unique_ptr<MemReq>>& reqQueue); 

std::unique_ptr<MemReq>& enqueueReq(int id, uint64_t addr, uint32_t len, int tag, int rw);

std::unique_ptr<MemReq> dequeueReq(std::deque<std::unique_ptr<MemReq>>& reqQueue);

extern int num_instr;

#endif
