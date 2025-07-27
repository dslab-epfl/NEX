#pragma once
#include <deque>
#include "../perf_sim/token_types.hh"

namespace protoacc{
    enum class CstStr {
        END_OF_MESSAGE=0,
        NONSCALAR=1,
        END_OF_MESSAGE_TOP_LEVEL=2,
        SCALAR=3,
        SUBMESSAGE=4,
        NONSUBMESSAGE=5,
        SCALAR_DISPATCH_REQ=0,
        STRING_GETPTR_REQ=1,
        STRING_LOADDATA_REQ=2,
        UNPACKED_REP_GETPTR_REQ=3,
        LOAD_NEW_SUBMESSAGE=4,
        LOAD_HASBITS_AND_IS_SUBMESSAGE=5,
        LOAD_EACH_FIELD=6,
        WRITE_OUT=7,
        READ=0,
        WRITE=1,
    };

    extern std::deque<token_class_iasbrr*> dma_read_requests;
    extern std::deque<token_class_iasbrr*> dma_write_requests;
    extern std::deque<token_class_iasbrr*> dma_read_resp;
    extern std::deque<token_class_iasbrr*> dma_write_resp;
}


extern std::vector<int> ids;