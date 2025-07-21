#pragma once
#include <deque>
#include <cstdint>
#include "sims/lpn/vta/perf_sim/token_types.hh"
#include "sims/lpn/vta/include/vta_bm.hh"

namespace vta{
    enum class CstStr {
        INP=0,
        LOADUOP=1,
        GEMM=2,
        SYNC=3,
        WGT=4,
        LOADACC=5,
        ALU=6,
        COMPUTE=7,
        FINISH=8,
        EMPTY=9,
        STORE=10,
        LOAD=11,
        DMA_LOAD_INP=0,
        DMA_LOAD_WGT=1,
        DMA_STORE=2,
        DMA_LOAD_ACC=3,
        DMA_LOAD_UOP=4,
        DMA_LOAD_INSN=5
    };
}

extern std::deque<token_class_iasbrr*> dma_read_requests;
extern std::deque<token_class_iasbrr*> dma_write_requests;
extern std::deque<token_class_iasbrr*> dma_read_resp;
extern std::deque<token_class_iasbrr*> dma_write_resp;

extern std::vector<int> ids;
extern VTABm* vta_bm_;
