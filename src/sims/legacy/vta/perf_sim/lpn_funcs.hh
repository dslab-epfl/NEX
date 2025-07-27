#pragma once
#include <cassert>
#include <stdlib.h>
#include <functional>
#include <math.h>
#include <algorithm>
#include <sys/types.h>
#include "place_transition.hh"
#include "places.hh"
#include "token_types.hh"
#include "../include/driver.hh"
#include <config/config.h>

namespace vta{  
    // in picoseconds
    uint64_t CYCLEPERIOD = 1000000/CONFIG_VTA_FREQ;
    
template<typename T>
int delay_alu(Place<T>* dependent_place) {
        auto uop_begin = dependent_place->tokens[0]->uop_begin;
        auto uop_end = dependent_place->tokens[0]->uop_end;
        auto lp_0 = dependent_place->tokens[0]->lp_0;
        auto lp_1 = dependent_place->tokens[0]->lp_1;
        auto use_alu_imm = dependent_place->tokens[0]->use_alu_imm;
        return ((1 + 5) + ((((uop_end - uop_begin) * lp_1) * lp_0) * (2 - use_alu_imm)))*vta::CYCLEPERIOD;
};
template<typename T>
int delay_gemm(Place<T>* dependent_place) {
        auto uop_begin = dependent_place->tokens[0]->uop_begin;
        auto uop_end = dependent_place->tokens[0]->uop_end;
        auto lp_0 = dependent_place->tokens[0]->lp_0;
        auto lp_1 = dependent_place->tokens[0]->lp_1;
        return ((1 + 5) + (((uop_end - uop_begin) * lp_1) * lp_0))*vta::CYCLEPERIOD;
};
template<typename T>
int delay_loadAcc(Place<T>* dependent_place) {
        auto xsize = dependent_place->tokens[0]->xsize;
        auto ysize = dependent_place->tokens[0]->ysize;
        return ((1 + 21) + ((xsize * 8) * ysize))*vta::CYCLEPERIOD;
};
template<typename T>
int delay_loadUop(Place<T>* dependent_place) {
        auto xsize = dependent_place->tokens[0]->xsize;
        auto ysize = dependent_place->tokens[0]->ysize;
        return ((1 + 21) + ((xsize * ((ysize + 2) - (ysize % 2))) /(double) 2))*vta::CYCLEPERIOD;
};
std::function<int()> take_1_token() {
    auto inp_weight = [&]() -> int {
        return 1;
    };
    return inp_weight;
};
template<typename T>
std::function<int()> arbiterhelperord_take_0_or_1(int my_idx, Place<T>* porder_keeper) {
    auto inp_weight = [&, my_idx ,porder_keeper]() -> int {
        auto cur_turn = porder_keeper->tokens[0]->idx;
        if (cur_turn == my_idx) {
            return 1;
        }
        else {
            return 0;
        }
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_readLen(Place<T>* dependent_place) {
    auto inp_weight = [&, dependent_place]() -> int {
        return dependent_place->tokens[0]->insn_count;
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_dep_pop_next(Place<T>* dependent_place) {
    auto inp_weight = [&, dependent_place]() -> int {
        if (dependent_place->tokens[0]->pop_next == 1) {
            return 1;
        }
        else {
            return 0;
        }
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_resp_token_load_module(Place<T>* insn_place) {
    auto inp_weight = [&, insn_place]() -> int {
        auto tstype = insn_place->tokens[0]->tstype;
        auto ysize = insn_place->tokens[0]->ysize;
        if ((tstype == (int)vta::CstStr::INP || tstype == (int)vta::CstStr::WGT)) {
            return ysize;
        }
        return 0;
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_dep_pop_prev(Place<T>* dependent_place) {
    auto inp_weight = [&, dependent_place]() -> int {
        if (dependent_place->tokens[0]->pop_prev == 1) {
            return 1;
        }
        else {
            return 0;
        }
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_resp_token_store_module(Place<T>* insn_place) {
    auto inp_weight = [&, insn_place]() -> int {
        auto subopcode = insn_place->tokens[0]->subopcode;
        auto xsize = insn_place->tokens[0]->xsize;
        auto ysize = insn_place->tokens[0]->ysize;
        if (subopcode != (int)vta::CstStr::SYNC) {
            return (xsize * ysize);
        }
        return 0;
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_resp_token_compute_module(Place<T>* insn_place) {
    auto inp_weight = [&, insn_place]() -> int {
        auto subopcode = insn_place->tokens[0]->subopcode;
        auto ysize = insn_place->tokens[0]->ysize;
        if ((subopcode == (int)vta::CstStr::LOADACC || subopcode == (int)vta::CstStr::LOADUOP)) {
            return ysize;
        }
        return 0;
    };
    return inp_weight;
};

std::function<bool()> psDrain_is_empty() {
    auto guard = [&]() -> bool {
        if(psDrain.tokensLen() == 0){
            return true;
        }
        return false;
    };
    return guard;
};

template<typename T, typename U>
std::function<void(BasePlace*)> arbiterhelperord_pass_turn_token(Place<T>* porder_keeper, std::vector<Place<U>*>& list_of_buf) {
    auto out_weight = [&, porder_keeper](BasePlace* output_place) -> void {
        auto cur_turn = porder_keeper->tokens[0]->idx;
        auto cur_buf = list_of_buf[cur_turn];
        auto token = cur_buf->tokens[0];
        output_place->pushToken(token);
    };
    return out_weight;
};
std::function<void(BasePlace*)> pass_empty_token() {
    auto out_weight = [&](BasePlace* output_place) -> void {
        NEW_TOKEN(EmptyToken, new_token);
        output_place->pushToken(new_token);
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> pass_token_match_port(Place<T>* from_place, int match_port) {
    auto out_weight = [&, from_place ,match_port](BasePlace* output_place) -> void {
        auto ref = from_place->tokens[0]->ref;
        auto token = from_place->tokens[0];
        if (ref == match_port) {
            output_place->pushToken(token);
        }
    };
    return out_weight;
};

std::function<void(BasePlace*)> call_get_mem(int type) {
    auto out_weight = [&, type](BasePlace* output_place) -> void {
        if(type == 0){
            assert(!dma_read_resp.empty());
            auto token = dma_read_resp.front(); 
            dma_read_resp.pop_front();
            output_place->pushToken(token);
        }else{
            assert(!dma_write_resp.empty());
            auto token = dma_write_resp.front();
            dma_write_resp.pop_front();
            output_place->pushToken(token);
        }
    };
    return out_weight;
};

template<typename T>
std::function<void(BasePlace*)> call_put_mem(Place<T>* from_place, int type) {
    auto out_weight = [&, from_place ,type](BasePlace* output_place) -> void {
        if(type == 0){
            dma_read_requests.push_back(from_place->tokens[0]);
        }else{
            dma_write_requests.push_back(from_place->tokens[0]);
        }
    };
    return out_weight;
};

std::function<uint64_t()> delay_0_if_resp_ready(int type) {
    auto delay = [&, type]() -> uint64_t {
        if(type == 0){
            if(dma_read_resp.size() > 0){
                return 0;
            }
        }else{
            if(dma_write_resp.size() > 0){
                return 0;
            }
        }
        return lpn::LARGE;
    };
    return delay;
};

template<typename T>
std::function<void(BasePlace*)> output_insn_read_cmd(Place<T>* from_place) {
    auto out_weight = [&, from_place](BasePlace* output_place) -> void {
        auto total_insn = from_place->tokens[0]->total_insn;
        auto max_insn = 128;
        auto ites = (total_insn / max_insn);
        auto remain = (total_insn % max_insn);
        for (int i = 0; i < ites; ++i) {
            NEW_TOKEN(token_class_insn_count, new_token);
            new_token->insn_count = max_insn;
            output_place->pushToken(new_token);
        }
        if (remain > 0) {
            NEW_TOKEN(token_class_insn_count, new_token);
            new_token->insn_count = remain;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> pass_token(Place<T>* from_place, int num) {
    auto out_weight = [&, from_place ,num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            auto token = from_place->tokens[i];
            output_place->pushToken(token);
        }
    };
    return out_weight;
};
std::function<void(BasePlace*)> mem_request(int port, int id, int num) {
    auto out_weight = [&, port ,id ,num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            auto tk = NEW_TOKEN_WO_DECL(token_class_iasbrr);
            tk->id = id;
            tk->addr = 0;
            tk->size = 0;
            tk->buffer = 0;
            tk->rw = 0;
            tk->ref = port;
            output_place->pushToken(tk);
        }
    };
    return out_weight;
};
std::function<void(BasePlace*)> push_request_order(int port, int num) {
    auto out_weight = [&, port ,num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            NEW_TOKEN(token_class_idx, new_token);
            new_token->idx = port;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T, typename U>
std::function<void(BasePlace*)> pass_var_token_readLen(Place<T>* from_place, Place<U>* dependent_place) {
    auto out_weight = [&, from_place ,dependent_place](BasePlace* output_place) -> void {
        auto num = dependent_place->tokens[0]->insn_count;
        for (int i = 0; i < num; ++i) {
            auto token = from_place->tokens[i];
            output_place->pushToken(token);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> mem_request_load_module(int port, Place<T>* insn_place) {
    auto out_weight = [&, port ,insn_place](BasePlace* output_place) -> void {
        auto tstype = insn_place->tokens[0]->tstype;
        auto ysize = insn_place->tokens[0]->ysize;
        if (tstype == (int)vta::CstStr::INP) {
            for (int i = 0; i < ysize; ++i) {
                NEW_TOKEN(token_class_iasbrr, new_token);
                new_token->id = (int)vta::CstStr::DMA_LOAD_INP;
                new_token->addr = 0;
                new_token->size = 0;
                new_token->buffer = 0;
                new_token->rw = 0;
                new_token->ref = port;
                output_place->pushToken(new_token);
            }
        }
        else {
            if (tstype == (int)vta::CstStr::WGT) {
                for (int i = 0; i < ysize; ++i) {
                    NEW_TOKEN(token_class_iasbrr, new_token);
                    new_token->id = (int)vta::CstStr::DMA_LOAD_WGT;
                    new_token->addr = 0;
                    new_token->size = 0;
                    new_token->buffer = 0;
                    new_token->rw = 0;
                    new_token->ref = port;
                    output_place->pushToken(new_token);
                }
            }
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> push_request_order_load_module(int port, Place<T>* insn_place) {
    auto out_weight = [&, port ,insn_place](BasePlace* output_place) -> void {
        auto tstype = insn_place->tokens[0]->tstype;
        auto ysize = insn_place->tokens[0]->ysize;
        if ((tstype == (int)vta::CstStr::INP || tstype == (int)vta::CstStr::WGT)) {
            for (int i = 0; i < ysize; ++i) {
                NEW_TOKEN(token_class_idx, new_token);
                new_token->idx = port;
                output_place->pushToken(new_token);
            }
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> output_dep_push_next(Place<T>* dependent_place) {
    auto out_weight = [&, dependent_place](BasePlace* output_place) -> void {
        auto direc = dependent_place->tokens[0]->push_next;
        if (direc == 1) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> mem_request_store_module(int port, Place<T>* insn_place) {
    auto out_weight = [&, port ,insn_place](BasePlace* output_place) -> void {
        auto subopcode = insn_place->tokens[0]->subopcode;
        auto xsize = insn_place->tokens[0]->xsize;
        auto ysize = insn_place->tokens[0]->ysize;
        if (subopcode != (int)vta::CstStr::SYNC) {
            for (int i = 0; i < (xsize * ysize); ++i) {
                NEW_TOKEN(token_class_iasbrr, new_token);
                new_token->id = (int)vta::CstStr::DMA_STORE;
                new_token->addr = 0;
                new_token->size = 0;
                new_token->buffer = 0;
                new_token->rw = 1;
                new_token->ref = port;
                output_place->pushToken(new_token);
            }
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> push_request_order_store_module(int port, Place<T>* insn_place) {
    auto out_weight = [&, port ,insn_place](BasePlace* output_place) -> void {
        auto subopcode = insn_place->tokens[0]->subopcode;
        auto xsize = insn_place->tokens[0]->xsize;
        auto ysize = insn_place->tokens[0]->ysize;
        if (subopcode != (int)vta::CstStr::SYNC) {
            for (int i = 0; i < (xsize * ysize); ++i) {
                NEW_TOKEN(token_class_idx, new_token);
                new_token->idx = port;
                output_place->pushToken(new_token);
            }
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> output_dep_push_prev(Place<T>* dependent_place) {
    auto out_weight = [&, dependent_place](BasePlace* output_place) -> void {
        auto direc = dependent_place->tokens[0]->push_prev;
        if (direc == 1) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> mem_request_compute_module(int port, Place<T>* insn_place) {
    auto out_weight = [&, port ,insn_place](BasePlace* output_place) -> void {
        auto subopcode = insn_place->tokens[0]->subopcode;
        auto ysize = insn_place->tokens[0]->ysize;
        if (subopcode == (int)vta::CstStr::LOADACC) {
            for (int i = 0; i < ysize; ++i) {
                NEW_TOKEN(token_class_iasbrr, new_token);
                new_token->id = (int)vta::CstStr::DMA_LOAD_ACC;
                new_token->addr = 0;
                new_token->size = 0;
                new_token->buffer = 0;
                new_token->rw = 0;
                new_token->ref = port;
                output_place->pushToken(new_token);
            }
        }
        else {
            if (subopcode == (int)vta::CstStr::LOADUOP) {
                for (int i = 0; i < ysize; ++i) {
                    NEW_TOKEN(token_class_iasbrr, new_token);
                    new_token->id = (int)vta::CstStr::DMA_LOAD_UOP;
                    new_token->addr = 0;
                    new_token->size = 0;
                    new_token->buffer = 0;
                    new_token->rw = 0;
                    new_token->ref = port;
                    output_place->pushToken(new_token);
                }
            }
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> push_request_order_compute_module(int port, Place<T>* insn_place) {
    auto out_weight = [&, port ,insn_place](BasePlace* output_place) -> void {
        auto subopcode = insn_place->tokens[0]->subopcode;
        auto ysize = insn_place->tokens[0]->ysize;
        if ((subopcode == (int)vta::CstStr::LOADACC || subopcode == (int)vta::CstStr::LOADUOP)) {
            for (int i = 0; i < ysize; ++i) {
                NEW_TOKEN(token_class_idx, new_token);
                new_token->idx = port;
                output_place->pushToken(new_token);
            }
        }
    };
    return out_weight;
};
template<typename T>
std::function<bool()> take_opcode_token(Place<T>* dependent_place, int opcode) {
    auto guard = [&, dependent_place ,opcode]() -> bool {
        if (dependent_place->tokens[0]->opcode != opcode) {
            return false;
        }
        return true;
    };
    return guard;
};
std::function<bool()> empty_guard() {
    auto guard = [&]() -> bool {
        return true;
    };
    return guard;
};
std::function<uint64_t()> con_delay_ns(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return scale*1000;
    };
    return delay;
};
std::function<uint64_t()> con_delay(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return scale*vta::CYCLEPERIOD;
    };
    return delay;
};

template<typename T>
std::function<uint64_t()> delay_load(Place<T>* dependent_place) {
    auto delay = [&, dependent_place]() -> uint64_t {
        auto subopcode = dependent_place->tokens[0]->subopcode;
        if (subopcode == (int)vta::CstStr::SYNC) {
            return 2*vta::CYCLEPERIOD;
        }
        return 0;
    };
    return delay;
};
template<typename T>
std::function<uint64_t()> delay_store(Place<T>* dependent_place) {
    auto delay = [&, dependent_place]() -> uint64_t {
        auto subopcode = dependent_place->tokens[0]->subopcode;
        if (subopcode == (int)vta::CstStr::SYNC) {
            return 2*vta::CYCLEPERIOD;
        }
        return 0;
    };
    return delay;
};
template<typename T>
std::function<uint64_t()> delay_compute(Place<T>* dependent_place) {
    auto delay = [&, dependent_place]() -> uint64_t {
        auto subopcode = dependent_place->tokens[0]->subopcode;
        if (subopcode == (int)vta::CstStr::SYNC) {
            return (1 + 1)*vta::CYCLEPERIOD;
        }
        if (subopcode == (int)vta::CstStr::ALU) {
            return     delay_alu(dependent_place);
        }
        if (subopcode == (int)vta::CstStr::GEMM) {
            return     delay_gemm(dependent_place);
        }
        if (subopcode == (int)vta::CstStr::LOADACC) {
            return 0;
        }
        if (subopcode == (int)vta::CstStr::LOADUOP) {
            return 0;
        }
        assert(0);
    };
    return delay;
};


template<typename T>
std::function<uint64_t()> simple_axi_delay(Place<T>* dependent_place) {
    auto delay = [&, dependent_place]() -> uint64_t {
        auto _delay = int(std::ceil(dependent_place->tokens[0]->size/8.0))*vta::CYCLEPERIOD;
        return _delay;
    };
    return delay;
};

std::function<uint64_t()> delay_latency_if_resp_ready(int type, int _latency) {
    auto delay = [&, type, _latency]() -> uint64_t {
        if(type == 0){
            if(dma_read_resp.size() > 0){
                return _latency*1000;
            }
        }else{
            if(dma_write_resp.size() > 0){
                return _latency*1000;
            }
        }
        return lpn::LARGE;
    };
    return delay;
};

}
