#pragma once
#include <deque>
#include <cstdint>
#include "sims/lpn/jpeg_decoder/perf_sim/token_types.hh"
#include "sims/lpn/jpeg_decoder/include/jpeg_bm.hh"

namespace jpeg{
    enum class CstStr {
        DMA_READ=0,
        DMA_WRITE=1,
        JPEG_SCALE_TO_PS=500,
    };
}

extern std::deque<token_class_iasbrr*> dma_read_requests;
extern std::deque<token_class_iasbrr*> dma_write_requests;
extern std::deque<token_class_iasbrr*> dma_read_resp;
extern std::deque<token_class_iasbrr*> dma_write_resp;

extern std::vector<int> ids;
extern JPEGBm* jpeg_bm_;
