#pragma once
#include <stdlib.h>
#include <functional>
#include <math.h>
#include <algorithm>
#include "place_transition.hh"
#include "places.hh"
#include "../include/driver.hh"

namespace jpeg{
template<typename T>
std::function<int()> special_edge(Place<T>* from_place) {
    auto inp_weight = [&, from_place]() -> int {
        auto num = from_place->tokens[0]->avg_block_size;
        return num;
    };
    return inp_weight;
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
std::function<int()> take_all(Place<T>* from_place) {
    auto inp_weight = [&, from_place]() -> int {
        auto wgt = from_place->tokensLen();
        if (wgt > 0) {
            return wgt;
        }
        else {
            return 1;
        }
    };
    return inp_weight;
};
std::function<int()> con_edge(int number) {
    auto inp_weight = [&, number]() -> int {
        return number;
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_max_16_tokens(Place<T>* from_place) {
    auto inp_weight = [&, from_place]() -> int {
        auto num = from_place->tokensLen();
        if (num == 0) {
            return 1;
        }
        if (num > 16) {
            return 16;
        }
        else {
            return num;
        }
    };
    return inp_weight;
};
template<typename T>
std::function<int()> take_resp_token(Place<T>* from_place) {
    auto inp_weight = [&, from_place]() -> int {
        auto num = from_place->tokens[0]->num;
        return num;
    };
    return inp_weight;
};
std::function<int()> take_resp_token_write(int num) {
    auto inp_weight = [&, num]() -> int {
        return num;
    };
    return inp_weight;
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


template<typename T>
std::function<void(BasePlace*)> pass_all_tokens(Place<T>* from_place) {
    auto out_weight = [&, from_place](BasePlace* output_place) -> void {
        auto num = from_place->tokensLen();
        for (int i = 0; i < num; ++i) {
            auto token = from_place->tokens[i];
            output_place->pushToken(token);
        }
    };
    return out_weight;
};
std::function<void(BasePlace*)> con_tokens(int number) {
    auto out_weight = [&, number](BasePlace* output_place) -> void {
        for (int i = 0; i < number; ++i) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> record_max_16(Place<T>* from_place) {
    auto out_weight = [&, from_place](BasePlace* output_place) -> void {
        auto num = from_place->tokensLen();
        if (num > 16) {
            num = 16;
        }
        NEW_TOKEN(jpeg::token_class_num, new_token);
        new_token->num = num;
        output_place->pushToken(new_token);
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> push_request_order(int port, Place<T>* from_place) {
    auto out_weight = [&, port ,from_place](BasePlace* output_place) -> void {
        auto num = from_place->tokensLen();
        if (num > 16) {
            num = 16;
        }
        for (int i = 0; i < num; ++i) {
            NEW_TOKEN(jpeg::token_class_idx, new_token);
            new_token->idx = port;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> mem_request(int port, int id, Place<T>* from_place) {
    auto out_weight = [&, port ,id ,from_place](BasePlace* output_place) -> void {
        auto num = from_place->tokensLen();
        if (num > 16) {
            num = 16;
        }
        for (int i = 0; i < num; ++i) {
            auto tk = NEW_TOKEN_WO_DECL(jpeg::token_class_iasbrr);
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
std::function<void(BasePlace*)> push_request_order_write(int port, int num) {
    auto out_weight = [&, port ,num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            NEW_TOKEN(jpeg::token_class_idx, new_token);
            new_token->idx = port;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
std::function<void(BasePlace*)> mem_request_write(int port, int id, int num) {
    auto out_weight = [&, port ,id ,num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            auto tk = NEW_TOKEN_WO_DECL(jpeg::token_class_iasbrr);
            tk->id = id;
            tk->addr = 0;
            tk->size = 0;
            tk->buffer = 0;
            tk->rw = 1;
            tk->ref = port;
            output_place->pushToken(tk);
        }
    };
    return out_weight;
};
template<typename T>
std::function<void(BasePlace*)> put_recv_buf(Place<T>* from_place) {
    auto out_weight = [&, from_place](BasePlace* output_place) -> void {
        auto num = from_place->tokens[0]->num;
        for (int i = 0; i < (num * 32); ++i) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
std::function<int()> const_threshold(int number) {
    auto threshold = [&, number]() -> int {
        return number;
    };
    return threshold;
};
std::function<uint64_t()> con_delay_ns(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return scale*1000;
    };
    return delay;
};
std::function<uint64_t()> con_delay(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return scale*(int)jpeg::CstStr::JPEG_SCALE_TO_PS;
    };
    return delay;
};

std::function<uint64_t()> constant_func(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return (scale * (int)jpeg::CstStr::JPEG_SCALE_TO_PS);
    };
    return delay;
};
template<typename T>
std::function<uint64_t()> mcu_delay(Place<T>* from_place) {
    auto delay = [&, from_place]() -> uint64_t {
        return (((from_place->tokens[0]->nonzero * 3) + 6) * (int)jpeg::CstStr::JPEG_SCALE_TO_PS);
    };
    return delay;
};

std::function<void(BasePlace*)> call_get_mem(int type) {
    auto out_weight = [&, type](BasePlace* output_place) -> void {
        if(type == 0){
            assert(!jpeg::dma_read_resp.empty());
            auto token = jpeg::dma_read_resp.front(); 
            jpeg::dma_read_resp.pop_front();
            output_place->pushToken(token);
        }else{
            assert(!jpeg::dma_write_resp.empty());
            auto token = jpeg::dma_write_resp.front();
            jpeg::dma_write_resp.pop_front();
            output_place->pushToken(token);
        }
    };
    return out_weight;
};

template<typename T>
std::function<void(BasePlace*)> call_put_mem(Place<T>* from_place, int type) {
    auto out_weight = [&, from_place ,type](BasePlace* output_place) -> void {
        if(type == 0){
            jpeg::dma_read_requests.push_back(from_place->tokens[0]);
        }else{
            jpeg::dma_write_requests.push_back(from_place->tokens[0]);
        }
    };
    return out_weight;
};

std::function<uint64_t()> delay_0_if_resp_ready(int type) {
    auto delay = [&, type]() -> uint64_t {
        if(type == 0){
            if(jpeg::dma_read_resp.size() > 0){
                return 0;
            }
        }else{
            if(jpeg::dma_write_resp.size() > 0){
                return 0;
            }
        }
        return lpn::LARGE;
    };
    return delay;
};
}
