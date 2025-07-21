#ifndef __TRANSITIONS__
#define __TRANSITIONS__
#include <stdlib.h>
#include <functional>
#include "place_transition.hh"
#include "places.hh"
#include "lpn_funcs.hh"

std::vector<Place<token_class_ralmtrd>*> list_10_devices = 
{
    &memory_resp_0,
    &memory_resp_1,
    &memory_resp_2,
    &memory_resp_3,
    &memory_resp_4,
    &memory_resp_5,
    &memory_resp_6,
    &memory_resp_7,
    &memory_resp_8,
    &memory_resp_9
};

// Transition dram_process_mem_req = {
//     .id = "dram_process_mem_req",
//     .delay_f = dram_delay(&dram_mem_req, (int)mem::CstStr::MEM_LATENCY, (int)mem::CstStr::MEM_ACCESS_BYTES),
//     .p_input = {&dram_mem_req},
//     .p_output = {&dram_mem_resp},  
//     .pi_w = {take_1_token()},
//     .po_w = {pass_token(&dram_mem_req, 1)},
//     .pi_w_threshold = {NULL},
//     .pi_guard = {NULL},
//     .pip = constant_func(1),
//     // .pip = {NULL},
// };

Transition tprocess_mem_req_front = {
    .id = "tprocess_mem_req_front",
    // don't set to 0, scx_pci will stuck
    .delay_f = con_delay(2),
    .p_input = {&memory_req_front},
    .p_output = {&memory_req_store,&memory_req_cl,&inflight_cl_cnt},  
    .pi_w = {take_1_token()},
    .po_w = {pass_token(&memory_req_front, 1),pass_cl_token(&memory_req_front),pass_cl_cnt(&memory_req_front)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition tlatency_sim = {
    .id = "tlatency_sim",
    .delay_f = cal_cache_delay(&memory_req_cl),
    .p_input = {&memory_req_cl,&memory_inflight_cap},
    .p_output = {&memory_resp_cl,&memory_inflight_cap},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {pass_token(&memory_req_cl, 1),pass_token(&memory_inflight_cap, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = con_delay(1)
};
Transition tforge_mem_resp = {
    .id = "tforge_mem_resp",
    .delay_f = delay_num_cl(&inflight_cl_cnt),
    .p_input = {&memory_req_store,&inflight_cl_cnt,&memory_resp_cl},
    .p_output = {&memory_resp},  
    .pi_w = {take_1_token(),take_1_token(),take_cl_cnt(&inflight_cl_cnt)},
    .po_w = {pass_token(&memory_req_store, 1)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};


Transition distributed_mem_resp = {
    .id = "distributed_mem_resp",
    .delay_f = con_delay(0),
    .p_input = {&memory_resp},
    .p_output = {&dummy_place},  
    .pi_w = {take_1_token()},
    .po_w = {distribute_mem_resp(&memory_resp, list_10_devices)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};

#endif
