#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

int num_instr;
std::map<int, std::deque<std::unique_ptr<MemReq>>> io_req_map;
std::map<int, std::deque<std::unique_ptr<MemReq>>> io_pending_map;
std::deque<token_class_iasbrr*> dma_read_requests;
std::deque<token_class_iasbrr*> dma_write_requests;
std::deque<token_class_iasbrr*> dma_read_resp;
std::deque<token_class_iasbrr*> dma_write_resp;

std::vector<int> ids = {
        (int)protoacc::CstStr::SCALAR_DISPATCH_REQ, 
        (int)protoacc::CstStr::STRING_GETPTR_REQ,
        (int)protoacc::CstStr::STRING_LOADDATA_REQ,
        (int)protoacc::CstStr::UNPACKED_REP_GETPTR_REQ,
        (int)protoacc::CstStr::LOAD_NEW_SUBMESSAGE,
        (int)protoacc::CstStr::LOAD_HASBITS_AND_IS_SUBMESSAGE,
        (int)protoacc::CstStr::LOAD_EACH_FIELD,
        (int)protoacc::CstStr::WRITE_OUT
};


void setupReqQueues(const std::vector<int>& ids) {
  for (const auto& id : ids) {
    io_req_map[id] = std::deque<std::unique_ptr<MemReq>>();
    io_pending_map[id] = std::deque<std::unique_ptr<MemReq>>();
  }
}


void ClearReqQueues(const std::vector<int>& ids) {
  for (const auto& id : ids) {
    io_req_map[id].clear();
    io_pending_map[id].clear();
  }
}

std::unique_ptr<MemReq>& frontReq(std::deque<std::unique_ptr<MemReq>>& reqQueue){
  return reqQueue.front();
}

std::unique_ptr<MemReq>& enqueueReq(int id, uint64_t addr, uint32_t len, int tag, int rw) {
  // printf("enqueueReq: id=%d, addr=%lx, len=%d, tag=%d, rw=%d\n", id, addr, len, tag, rw);
  auto req = std::make_unique<MemReq>();
  req->addr = addr;
  req->tag = tag;
  req->id = id;
  req->rw = rw;
  req->len = len;
  auto& reqQueue = io_req_map[tag];
  reqQueue.push_back(std::move(req));
  // printf("enqueueReq done\n");
  return reqQueue.back();
}

std::unique_ptr<MemReq> dequeueReq(std::deque<std::unique_ptr<MemReq>>& reqQueue) {
  std::unique_ptr<MemReq> req = std::move(reqQueue.front());
  reqQueue.pop_front();
  return req;
}
