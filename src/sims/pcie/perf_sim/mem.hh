#include <deque>
#include "token_types.hh"

extern std::deque<token_class_abirrs*> dma_read_requests;
extern std::deque<token_class_abirrs*> dma_write_requests;
extern std::deque<token_class_abirrs*> dma_read_resp;
extern std::deque<token_class_abirrs*> dma_write_resp;
