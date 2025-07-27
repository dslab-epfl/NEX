#pragma once
#include <cstdint>
#include <stdlib.h>
#include <functional>
#include <math.h>
#include <algorithm>
#include "place_transition.hh"
#include "places.hh"
#include "mem.hh"

extern "C" {
    void mem_advance_until_time(uint64_t ts);
    int mem_active(uint64_t* next_active_ts);
    int mem_put(uint64_t virtual_ts, uint64_t req_id, uint64_t addr, uint64_t len, uint64_t mem_buffer_ptr, int dma_fd, int type, uint64_t ref_ptr);
    int mem_get(uint64_t* ref_ptr, uint64_t* req_id, uint64_t* buffer, uint64_t* len);
}

namespace pcie{
    enum class CstStr {
    D0=0,
    D1=1,
    D2=2,
    D3=3,
    D4=4,
    D5=5,
    D6=6,
    D7=7,
    D8=8,
    D9=9,
    D10=10,
    D11=11,
    D12=12,
    D13=13,
    D14=14,
    D15=15,
    D16=16,
    D17=17,
    D18=18,
    READ=0,
    WRITE=1,
    CMPL=20,
    POST=21,
    NONPOST=22,
    };
}

static std::function<int()> con_edge(int number) {
    auto inp_weight = [&, number]() -> int {
        return number;
    };
    return inp_weight;
};
static std::function<int()> take_some_token(int num) {
    auto inp_weight = [&, num]() -> int {
        return num;
    };
    return inp_weight;
};
static std::function<int()> take_1_token() {
    auto inp_weight = [&]() -> int {
        return 1;
    };
    return inp_weight;
};
template<typename T, typename U>
static std::function<int()> arbiterhelper_take_0_or_1(int my_idx, std::vector<Place<T>*>& list_of_buf, Place<U>* pidx) {
    auto inp_weight = [&, my_idx ,pidx]() -> int {
        auto total = list_of_buf.size();
        std::vector<int>  tk_status ;
        for (auto p : list_of_buf) {
            tk_status.push_back(p->tokensLen());
        }
        auto cur_turn = pidx->tokens[0]->idx;
        std::vector<int>  _l ;
        for (int i = cur_turn; i < total; ++i) {
            _l.push_back(i);
        }
        for (int i = 0; i < cur_turn; ++i) {
            _l.push_back(i);
        }
        auto earliest_enabled_idx = -(1);
        for (auto i : _l) {
            if (tk_status[i] > 0) {
                earliest_enabled_idx = i;
                break;
            }
        }
        if (earliest_enabled_idx == -(1)) {
            if (cur_turn == my_idx) {
                return 1;
            }
            else {
                return 0;
            }
        }
        else {
            if (earliest_enabled_idx == my_idx) {
                return 1;
            }
            else {
                return 0;
            }
        }
    };
    return inp_weight;
};
template<typename T>
static std::function<int()> take_credit_token(Place<T>* from_place, int unit) {
    auto inp_weight = [&, from_place ,unit]() -> int {
        auto num_credit = std::max(1, (int)ceil((from_place->tokens[0]->pkt_size /(double) unit)));
        return num_credit;
    };
    return inp_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_atomic_smaller_token(int tlp_size, Place<T>* from_place) {
    auto out_weight = [&, tlp_size ,from_place](BasePlace* output_place) -> void {
        // this should only apply to write requests
        // where packet size is the same as requested size
        auto num_atomic = (int)ceil((from_place->tokens[0]->pkt_size /(double) tlp_size));
        auto remain = from_place->tokens[0]->pkt_size;
        auto offset = from_place->tokens[0]->offset;
        auto dataref = from_place->tokens[0]->dataref;
        auto src = from_place->tokens[0]->src;
        auto dst = from_place->tokens[0]->dst;
        auto type = from_place->tokens[0]->type;
        auto requested_size = from_place->tokens[0]->requested_size;
        if(type == 1){
            assert(remain == requested_size);
        }
        for (int i = 0; i < num_atomic; ++i) {
            auto this_size = std::min(tlp_size, remain);
            remain = (remain - this_size);
            auto token = NEW_TOKEN_WO_DECL(token_class_ddopst);
            token->src = src;
            token->dst = dst;
            token->type = type;
            token->pkt_size = this_size;
            if(type == 1){
                token->requested_size = this_size;
            }else{
                // for read/cmpl, the packet won't be split up
                assert(num_atomic == 1);
                token->requested_size = requested_size;
            }
            token->offset = offset;
            token->dataref = dataref;
            output_place->pushToken(token);
            offset += this_size;
        }
    };
    return out_weight;
};
template<typename T, typename U>
static std::function<void(BasePlace*)> arbiterhelper_update_cur_turn(Place<T>* pidx, std::vector<Place<U>*>& list_of_buf) {
    auto out_weight = [&, pidx](BasePlace* output_place) -> void {
        auto total = list_of_buf.size();
        std::vector<int>  tk_status ;
        for (auto p : list_of_buf) {
            tk_status.push_back(p->tokensLen());
        }
        auto cur_turn = pidx->tokens[0]->idx;
        auto next_turn = cur_turn;
        std::vector<int>  _l ;
        for (int i = cur_turn; i < total; ++i) {
            _l.push_back(i);
        }
        for (int i = 0; i < cur_turn; ++i) {
            _l.push_back(i);
        }
        for (auto i : _l) {
            if (tk_status[i] > 0) {
                next_turn = ((i + 1) % total);
                break;
            }
        }
        NEW_TOKEN(token_class_idx, new_token);
        new_token->idx = next_turn;
        output_place->pushToken(new_token);
    };
    return out_weight;
};
template<typename T, typename U>
static std::function<void(BasePlace*)> arbiterhelpernocapwport_pass_turn_token(Place<T>* pidx, std::vector<Place<U>*>& list_of_buf, std::vector<int>& in_post_list) {
    auto out_weight = [&, pidx](BasePlace* output_place) -> void {
        auto total = list_of_buf.size();
        std::vector<int>  tk_status ;
        for (auto p : list_of_buf) {
            tk_status.push_back(p->tokensLen());
        }
        auto cur_turn = pidx->tokens[0]->idx;
        std::vector<int>  _l ;
        for (int i = cur_turn; i < total; ++i) {
            _l.push_back(i);
        }
        for (int i = 0; i < cur_turn; ++i) {
            _l.push_back(i);
        }
        auto cur_buf = list_of_buf[0];
        auto cur_port = 0;
        for (auto i : _l) {
            if (tk_status[i] > 0) {
                cur_buf = list_of_buf[i];
                cur_port = in_post_list[i];
                break;
            }
        }
        auto token = NEW_TOKEN_WO_DECL(token_class_ddoppst);
        token->dst = cur_buf->tokens[0]->dst;
        token->src = cur_buf->tokens[0]->src;
        token->type = cur_buf->tokens[0]->type;
        token->pkt_size = cur_buf->tokens[0]->pkt_size;
        token->offset = cur_buf->tokens[0]->offset;
        token->dataref = cur_buf->tokens[0]->dataref;
        token->requested_size = cur_buf->tokens[0]->requested_size;
        token->port = cur_port;
        output_place->pushToken(token);
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> increase_credit(Place<T>* send_buf, int this_port, int unit) {
    auto out_weight = [&, send_buf ,this_port ,unit](BasePlace* output_place) -> void {
        if (send_buf->tokens[0]->port == this_port) {
            auto num_credit =     std::max(1, (int)ceil((send_buf->tokens[0]->pkt_size /(double) unit)));
            for (int i = 0; i < num_credit; ++i) {
                NEW_TOKEN(EmptyToken, new_token);
                output_place->pushToken(new_token);
            }
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_credit_with_header_token(Place<T>* from_place, int header_bytes, int unit) {
    auto out_weight = [&, from_place ,header_bytes ,unit](BasePlace* output_place) -> void {
        auto num_credit = std::max(1, (int)ceil(((from_place->tokens[0]->pkt_size + header_bytes) /(double) unit)));
        for (int i = 0; i < num_credit; ++i) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_add_header_token(Place<T>* from_place, int num, int header_bytes) {
    auto out_weight = [&, from_place ,num ,header_bytes](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            auto token = NEW_TOKEN_WO_DECL(token_class_ddopst);
            token->src = from_place->tokens[i]->src;
            token->dst = from_place->tokens[i]->dst;
            token->type = from_place->tokens[i]->type;
            token->pkt_size = (from_place->tokens[i]->pkt_size + header_bytes);
            token->offset = from_place->tokens[i]->offset;
            token->requested_size = from_place->tokens[i]->requested_size;
            token->dataref = from_place->tokens[i]->dataref;
            output_place->pushToken(token);
        }
    };
    return out_weight;
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
static std::function<void(BasePlace*)> pass_empty_token(int num) {
    auto out_weight = [&, num](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> store_cmpl(Place<T>* buf_place) {
    auto out_weight = [&, buf_place](BasePlace* output_place) -> void {
        auto type = buf_place->tokens[0]->type;
        if (type == (int)pcie::CstStr::CMPL) {
            NEW_TOKEN(token_class_ddopst, new_token);
            new_token->src = buf_place->tokens[0]->src;
            new_token->dst = buf_place->tokens[0]->dst;
            new_token->type = type;
            new_token->pkt_size = buf_place->tokens[0]->pkt_size;
            new_token->offset = buf_place->tokens[0]->offset;
            new_token->dataref = buf_place->tokens[0]->dataref;
            new_token->requested_size = buf_place->tokens[0]->requested_size;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};

template<typename T>
static std::function<int()>take_1_if_mem_req(Place<T>* buf_place){
    auto inp_weight = [&, buf_place]() -> int {
        auto type = buf_place->tokens[0]->type;
        if ((type == (int)pcie::CstStr::READ || type == (int)pcie::CstStr::WRITE)){
            return 1;
        };
        return 0;
    };
    return inp_weight;
}

template<typename T>
static std::function<void(BasePlace*)> store_request(Place<T>* buf_place) {
    auto out_weight = [&, buf_place](BasePlace* output_place) -> void {
        auto type = buf_place->tokens[0]->type;
        if ((type == (int)pcie::CstStr::READ || type == (int)pcie::CstStr::WRITE)) {
            NEW_TOKEN(token_class_ddopst, new_token);
            new_token->src = buf_place->tokens[0]->src;
            new_token->dst = buf_place->tokens[0]->dst;
            new_token->type = type;
            new_token->pkt_size = buf_place->tokens[0]->pkt_size;
            new_token->offset = buf_place->tokens[0]->offset;
            new_token->dataref = buf_place->tokens[0]->dataref;
            new_token->requested_size = buf_place->tokens[0]->requested_size;
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> increase_credit_wc(Place<T>* send_buf, int unit) {
    auto out_weight = [&, send_buf ,unit](BasePlace* output_place) -> void {
        auto num_credit = std::max(1, (int)ceil((send_buf->tokens[0]->pkt_size /(double) unit)));
        for (int i = 0; i < num_credit; ++i) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T, typename U>
static std::function<void(BasePlace*)> arbiterhelper_pass_turn_token(Place<T>* pidx, std::vector<Place<U>*>& list_of_buf) {
    auto out_weight = [&, pidx](BasePlace* output_place) -> void {
        auto total = list_of_buf.size();
        std::vector<int>  tk_status ;
        for (auto p : list_of_buf) {
            tk_status.push_back(p->tokensLen());
        }
        auto cur_turn = pidx->tokens[0]->idx;
        std::vector<int>  _l ;
        for (int i = cur_turn; i < total; ++i) {
            _l.push_back(i);
        }
        for (int i = 0; i < cur_turn; ++i) {
            _l.push_back(i);
        }
        auto cur_buf = list_of_buf[0];
        for (auto i : _l) {
            if (tk_status[i] > 0) {
                cur_buf = list_of_buf[i];
                break;
            }
        }
        auto token = cur_buf->tokens[0];
        output_place->pushToken(token);
    };
    return out_weight;
};
template<typename T, typename U>
static std::function<void(BasePlace*)> arbiterhelper_pass_turn_empty_token(Place<T>* pidx, int idx, std::vector<Place<U>*>& list_of_buf) {
    auto out_weight = [&, pidx ,idx](BasePlace* output_place) -> void {
        auto total = list_of_buf.size();
        std::vector<int>  tk_status ;
        for (auto p : list_of_buf) {
            tk_status.push_back(p->tokensLen());
        }
        auto cur_turn = pidx->tokens[0]->idx;
        std::vector<int>  _l ;
        for (int i = cur_turn; i < total; ++i) {
            _l.push_back(i);
        }
        for (int i = 0; i < cur_turn; ++i) {
            _l.push_back(i);
        }
        auto earliest_enabled_idx = -(1);
        for (auto i : _l) {
            if (tk_status[i] > 0) {
                earliest_enabled_idx = i;
                break;
            }
        }
        if (earliest_enabled_idx == idx) {
            NEW_TOKEN(EmptyToken, new_token);
            output_place->pushToken(new_token);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> mem_request_read(Place<T>* from_place, int mps, int port) {
    auto out_weight = [&, from_place ,mps ,port](BasePlace* output_place) -> void {
        auto type = from_place->tokens[0]->type;
        auto dataref = from_place->tokens[0]->dataref;
        auto req_from = from_place->tokens[0]->src;
        auto src = from_place->tokens[0]->dst;
        auto dst = from_place->tokens[0]->src;
        auto pkt_size = from_place->tokens[0]->pkt_size;
        auto offset = from_place->tokens[0]->offset;
        auto req_size = from_place->tokens[0]->requested_size;
        assert(type != (int)pcie::CstStr::CMPL);
        if (type == (int)pcie::CstStr::READ) {
            // int req_size = ((token_class_ralmtra*)dataref)->len;
            auto remain = req_size;
            auto transfer_count = (int)ceil((req_size /(double) mps));
            // printf("transfer_count: %d, req_size %d, mps %d\n", transfer_count, req_size, mps);
            assert(transfer_count == 1);
            for (int i = 0; i < transfer_count; ++i) {
                auto this_size = std::min(mps, remain);
                auto realdata = (token_class_ralmtra*)(dataref);
                // realdata->subreq_cnt += 1;
                auto id = NEW_TOKEN_WO_DECL(token_class_ddopst);
                id->src = src;
                id->dst = dst;
                id->type = (int)pcie::CstStr::CMPL;
                id->pkt_size = this_size;
                id->requested_size = this_size;
                id->offset = offset;
                id->dataref = dataref;
                auto tk = NEW_TOKEN_WO_DECL(token_class_abirrs);
                tk->id = (uint64_t)id;
                assert(id != NULL);
                assert(tk->id != 0);
                tk->addr = offset+realdata->addr;
                tk->size = this_size;
                tk->buffer = 0;
                tk->rw = 0;
                tk->ref = port;
                output_place->pushToken(tk);
                offset = (offset + this_size);
            }
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> mem_request_write(Place<T>* from_place, int mps, int port) {
    auto out_weight = [&, from_place ,mps ,port](BasePlace* output_place) -> void {
        auto type = from_place->tokens[0]->type;
        auto dataref = from_place->tokens[0]->dataref;
        auto req_from = from_place->tokens[0]->src;
        auto src = from_place->tokens[0]->dst;
        auto dst = from_place->tokens[0]->src;
        auto pkt_size = from_place->tokens[0]->pkt_size;
        auto offset = from_place->tokens[0]->offset;
        assert(type != (int)pcie::CstStr::CMPL);
        if (type == (int)pcie::CstStr::WRITE) {
            auto realdata = (token_class_ralmtra*)(dataref);
            // realdata->subreq_cnt += 1;
            auto id = NEW_TOKEN_WO_DECL(token_class_ddopst);
            id->src = src;
            id->dst = dst;
            id->type = (int)pcie::CstStr::CMPL;
            id->pkt_size = 16;
            id->requested_size = pkt_size;
            id->offset = offset;
            id->dataref = dataref;
            auto tk = NEW_TOKEN_WO_DECL(token_class_abirrs);
            tk->id = (uint64_t)id;
            tk->addr = offset+realdata->addr;
            tk->size = pkt_size;
            tk->buffer = 0;
            tk->rw = 1;
            tk->ref = port;
            output_place->pushToken(tk);
        }
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> send_cmpl(Place<T>* from_place) {
    auto out_weight = [&, from_place](BasePlace* output_place) -> void {
        auto tk = (BaseToken*)(from_place->tokens[0]->id);
        output_place->pushToken(tk);
    };
    return out_weight;
};
template<typename T>
static std::function<void(BasePlace*)> pass_token_match_port(Place<T>* from_place, int match_port) {
    auto out_weight = [&, from_place ,match_port](BasePlace* output_place) -> void {
        auto ref = from_place->tokens[0]->ref;
        auto token = from_place->tokens[0];
        if (ref == match_port) {
            output_place->pushToken(token);
        }
    };
    return out_weight;
};


static std::function<void(BasePlace*)> call_get_mem(int type) {
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
static std::function<void(BasePlace*)> call_put_mem(Place<T>* from_place, int type) {
    auto out_weight = [&, from_place ,type](BasePlace* output_place) -> void {
        if(type == 0){
            dma_read_requests.push_back(from_place->tokens[0]);
        }else{
            dma_write_requests.push_back(from_place->tokens[0]);
        }
    };
    return out_weight;
};


template<typename T>
static std::function<void(BasePlace*)> pass_rm_header_token(Place<T>* from_place, int num, int header_bytes) {
    auto out_weight = [&, from_place ,num ,header_bytes](BasePlace* output_place) -> void {
        for (int i = 0; i < num; ++i) {
            auto token = NEW_TOKEN_WO_DECL(token_class_ddopst);
            token->src = from_place->tokens[i]->src;
            token->dst = from_place->tokens[i]->dst;
            token->type = from_place->tokens[i]->type;
            token->pkt_size = (from_place->tokens[i]->pkt_size - header_bytes);
            token->offset = from_place->tokens[i]->offset;
            token->dataref = from_place->tokens[i]->dataref;
            token->requested_size = from_place->tokens[i]->requested_size;
            output_place->pushToken(token);
        }
    };
    return out_weight;
};
static std::function<int()> empty_threshold() {
    auto threshold = [&]() -> int {
        return 0;
    };
    return threshold;
};
template<typename T>
static std::function<bool()> take_out_port(Place<T>* from_place, std::map<int, int>& routinfo, int this_port) {
    auto guard = [&, from_place ,this_port]() -> bool {
        auto dst = from_place->tokens[0]->dst;
        auto out_port = routinfo[dst];
        if (out_port == this_port) {
            return true;
        }
        else {
            return false;
        }
    };
    return guard;
};
static std::function<bool()> empty_guard() {
    auto guard = [&]() -> bool {
        return true;
    };
    return guard;
};
static std::function<uint64_t()> con_delay_ns(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return scale*1000;
    };
    return delay;
};
static std::function<uint64_t()> con_delay(int scale) {
    auto delay = [&, scale]() -> uint64_t {
        return scale*1000;
    };
    return delay;
};
template<typename T>
static std::function<uint64_t()> var_tlp_process_delay(Place<T>* from_place, int base) {
    auto delay = [&, from_place ,base]() -> uint64_t {
        auto pkt_size = from_place->tokens[0]->pkt_size;
        return (((int)ceil((pkt_size /(double) 16)) * 2) + base)*1000;
    };
    return delay;
};


template<typename T>
static std::function<uint64_t()> merge_delay(Place<T>* from_place) {
    auto delay = [&, from_place]() -> uint64_t {
        auto pkt_size = from_place->tokens[0]->pkt_size;
        return (int)ceil((pkt_size /(double) 64))*1000;
    };
    return delay;
};

static std::function<uint64_t()> delay_0_if_resp_ready(int type) {
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