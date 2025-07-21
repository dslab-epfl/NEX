#pragma once
#include <stdlib.h>
#include <functional>
#include <math.h>
#include <algorithm>
#include "place_transition.hh"
#include "places.hh"
#include "cache.hh"

namespace mem{
    enum class CstStr {
        IN_PICO=1000
    };
}

template<typename T>
static std::function<int()> take_cl_cnt(Place<T>* input_place) {
    auto inp_weight = [&, input_place]() -> int {
        return input_place->tokens[0]->num;
    };
    return inp_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_token(Place<T>* from_place, int num) {
    auto out_weight = [&, from_place ,num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            auto token = from_place->tokens[i];
            output_place->pushToken(token);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_cl_token(Place<T>* input_place) {
    auto out_weight = [&, input_place](BasePlace* output_place) -> void {
        auto addr = input_place->tokens[0]->addr;
        auto size = input_place->tokens[0]->len;
        auto rw = input_place->tokens[0]->type;
        auto num = 0;
        if (size == 0) {
            num = 1;
        }
        else {
            num = (size / 64);
            if(size % 64 != 0){
                num++;
            }
        }
        for (int i = 0; i < num; ++i) {
            NEW_TOKEN(token_class_addr_type, new_token);
            new_token->addr = (addr + (i * 64));
            new_token->type = rw;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_cl_cnt(Place<T>* input_place) {
    auto out_weight = [&, input_place](BasePlace* output_place) -> void {
        auto size = input_place->tokens[0]->len;
        auto num = 0;
        if (size == 0) {
            num = 1;
        }
        else {
            num = (size / 64);
            if(size % 64 != 0){
                num++;
            }
        }
        NEW_TOKEN(token_class_num, new_token);
        new_token->num = num;
        output_place->pushToken(new_token);
    };
    return out_weight;
};
static std::function<uint64_t()> con_delay(int constant) {
    auto delay = [&, constant]() -> uint64_t {
        return constant*(int)mem::CstStr::IN_PICO;
    };
    return delay;
};
template<typename T>
static std::function<uint64_t()> delay_num_cl(Place<T>* input_place) {
    auto delay = [&, input_place]() -> uint64_t {
        return (input_place->tokens[0]->num)*(int)mem::CstStr::IN_PICO;
    };
    return delay;
};

static std::function<int()> take_1_token() {
    auto inp_weight = [&]() -> int {
        return 1;
    };
    return inp_weight;
};

template<typename T>
static std::function<uint64_t()> cal_cache_delay(Place<T>* from_place) {
    auto delay = [&, from_place]() -> uint64_t {
        // return 0;
        uint64_t addr = from_place->tokens[0]->addr;
        return (int)mem::CstStr::IN_PICO*cache_sim.simulate_cache_access(addr);
    };
    return delay;
};

template<typename T>
static std::function<void(BasePlace*)> distribute_mem_resp(Place<T>* resp, std::vector<Place<T>*>& list_of_buf) {
    auto out_weight = [&, resp](BasePlace* output_place) -> void {
        auto id = resp->tokens[0]->dev_id;
        list_of_buf[id]->pushToken(resp->tokens[0]);
    };
    return out_weight;
};
  
