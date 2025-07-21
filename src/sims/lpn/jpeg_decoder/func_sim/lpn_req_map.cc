#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

int num_instr;
std::map<int, std::deque<std::unique_ptr<MemReq>>> io_req_map;
std::map<int, std::deque<std::unique_ptr<MemReq>>> io_send_req_map;
std::map<int, std::deque<std::unique_ptr<MemReq>>> io_pending_req_map;
std::deque<token_class_iasbrr*> dma_read_requests;
std::deque<token_class_iasbrr*> dma_write_requests;
std::deque<token_class_iasbrr*> dma_read_resp;
std::deque<token_class_iasbrr*> dma_write_resp;

std::vector<int> ids = {
        (int)jpeg::CstStr::DMA_READ, 
        (int)jpeg::CstStr::DMA_WRITE,
};

void setupReqQueues(const std::vector<int>& ids) {
  for (const auto& id : ids) {
    io_req_map[id] = std::deque<std::unique_ptr<MemReq>>();
    io_send_req_map[id] = std::deque<std::unique_ptr<MemReq>>();
    io_pending_req_map[id] = std::deque<std::unique_ptr<MemReq>>();
  }
}

void ClearReqQueues(const std::vector<int>& ids) {
  for (const auto& id : ids) {
    io_req_map[id].clear();
    io_send_req_map[id].clear();
    io_pending_req_map[id].clear();
  }
}

std::unique_ptr<MemReq>& frontReq(std::deque<std::unique_ptr<MemReq>>& reqQueue){
  return reqQueue.front();
}

std::unique_ptr<MemReq>& enqueueReq(int id, uint64_t addr, uint32_t len, int tag, int rw, void* buffer) {
  auto req = std::make_unique<MemReq>();
  req->addr = addr;
  req->tag = tag;
  req->id = id;
  req->rw = rw;
  req->len = len;
  req->buffer = buffer;
  auto& req_queue = io_req_map[tag];
  req_queue.push_back(std::move(req));
  return req_queue.back();
}

std::unique_ptr<MemReq> dequeueReq(std::deque<std::unique_ptr<MemReq>>& reqQueue) {
  std::unique_ptr<MemReq> req = std::move(reqQueue.front());
  reqQueue.pop_front();
  return req;
}

void putData(uint64_t addr, uint32_t len, int tag, int rw, uint64_t ts, void* buffer) {
  std::deque<std::unique_ptr<MemReq>>& reqs = io_pending_req_map[tag];
  auto it = reqs.begin();
  while (it != reqs.end()) {
    auto req = it->get();
    // Check bounds
    if(addr >= req->addr && addr + len <= req->addr + req->len){
      // memcpy(req->buffer+addr-req->addr, buffer, len);
      req->acquired_len += len;
      if(req->acquired_len == req->len){
        auto tk = static_cast<token_class_iasbrr*>(req->extra_ptr);
        tk->ts = ts;
        if(req->rw == READ_REQ){
          dma_read_resp.push_back(tk);
        } else {
          dma_write_resp.push_back(tk);
        }
        io_pending_req_map[tag].erase(it);
      }
      break;
    }
    ++it;
  }
}

