#pragma once
#include "place_transition.hh"
#include "token_types.hh"

extern Place<token_class_addr_type> memory_resp_cl;
extern Place<token_class_ralmtrd> memory_resp;
extern Place<token_class_ralmtrd> memory_req_front;
extern Place<token_class_num> inflight_cl_cnt;
extern Place<token_class_addr_type> memory_req_cl;
extern Place<> memory_inflight_cap;
extern Place<token_class_ralmtrd> memory_req_store;

extern Place<token_class_ralmtrd> memory_resp_0;
extern Place<token_class_ralmtrd> memory_resp_1;
extern Place<token_class_ralmtrd> memory_resp_2;
extern Place<token_class_ralmtrd> memory_resp_3;
extern Place<token_class_ralmtrd> memory_resp_4;
extern Place<token_class_ralmtrd> memory_resp_5;
extern Place<token_class_ralmtrd> memory_resp_6;
extern Place<token_class_ralmtrd> memory_resp_7;
extern Place<token_class_ralmtrd> memory_resp_8;
extern Place<token_class_ralmtrd> memory_resp_9;

extern Place<> dummy_place;