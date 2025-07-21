#pragma once
#include <stdlib.h>
#include <functional>
#include "places.hh"
#include "lpn_funcs.hh"
#include <config/config.h>

#if CONFIG_PCIE_LPN
    #define PCIE_LATENCY 0    
#else
    #define PCIE_LATENCY 400    
#endif


namespace jpeg {

std::vector<Place<jpeg::token_class_iasbrr>*> list_0 = {&jpeg::dma_read_req_put_0};
std::vector<Place<jpeg::token_class_iasbrr>*> list_1 = {&jpeg::dma_write_req_put_0};

Transition dma_read_arbiter = {
    .id = "dma_read_arbiter",
    .delay_f = con_delay(0),
    .p_input = {&jpeg::dma_read_fifo_order,&jpeg::dma_read_req_put_0},
    .p_output = {&jpeg::dma_read_mem_put_buf},  
    .pi_w = {jpeg::take_1_token(),jpeg::arbiterhelperord_take_0_or_1(0, &jpeg::dma_read_fifo_order)},
    .po_w = {jpeg::arbiterhelperord_pass_turn_token(&jpeg::dma_read_fifo_order, jpeg::list_0)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition dma_read_mem_get = {
    .id = "dma_read_mem_get",
    .delay_f = jpeg::con_delay_ns(PCIE_LATENCY),
    .p_input = {&jpeg::dma_read_recv_buf},
    .p_output = {&jpeg::dma_read_send_cap,&jpeg::dma_read_req_get_0},  
    .pi_w = {jpeg::take_1_token()},
    .po_w = {jpeg::pass_empty_token(), jpeg::pass_token_match_port(&jpeg::dma_read_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = jpeg::con_delay_ns(0)
};
Transition dma_read_recv_from_mem = {
    .id = "dma_read_recv_from_mem",
    .delay_f = jpeg::delay_0_if_resp_ready(0),
    .p_input = {},
    .p_output = {&jpeg::dma_read_recv_buf},  
    .pi_w = {},
    .po_w = {jpeg::call_get_mem(0)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = NULL
};
Transition dma_read_mem_put = {
    .id = "dma_read_mem_put",
    .delay_f = jpeg::con_delay_ns(PCIE_LATENCY),
    .p_input = {&jpeg::dma_read_mem_put_buf,&jpeg::dma_read_send_cap},
    .p_output = {&jpeg::dma_read_SINK},  
    .pi_w = {jpeg::take_1_token(),jpeg::take_1_token()},
    .po_w = {jpeg::call_put_mem(&jpeg::dma_read_mem_put_buf, 0)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = jpeg::con_delay_ns(0)
};
Transition dma_write_arbiter = {
    .id = "dma_write_arbiter",
    .delay_f = jpeg::con_delay(0),
    .p_input = {&jpeg::dma_write_fifo_order,&jpeg::dma_write_req_put_0},
    .p_output = {&jpeg::dma_write_mem_put_buf},  
    .pi_w = {jpeg::take_1_token(),jpeg::arbiterhelperord_take_0_or_1(0, &jpeg::dma_write_fifo_order)},
    .po_w = {jpeg::arbiterhelperord_pass_turn_token(&jpeg::dma_write_fifo_order, jpeg::list_1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition dma_write_mem_get = {
    .id = "dma_write_mem_get",
    .delay_f = jpeg::con_delay_ns(PCIE_LATENCY),
    .p_input = {&jpeg::dma_write_recv_buf},
    .p_output = {&jpeg::dma_write_send_cap,&jpeg::dma_write_req_get_0},  
    .pi_w = {jpeg::take_1_token()},
    .po_w = {jpeg::pass_empty_token(),jpeg::pass_token_match_port(&jpeg::dma_write_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = jpeg::con_delay_ns(0)
};
Transition dma_write_recv_from_mem = {
    .id = "dma_write_recv_from_mem",
    .delay_f = jpeg::delay_0_if_resp_ready(1),
    .p_input = {},
    .p_output = {&jpeg::dma_write_recv_buf},  
    .pi_w = {},
    .po_w = {jpeg::call_get_mem(1)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = NULL
};
Transition dma_write_mem_put = {
    .id = "dma_write_mem_put",
    .delay_f = jpeg::con_delay_ns(PCIE_LATENCY),
    .p_input = {&jpeg::dma_write_mem_put_buf,&jpeg::dma_write_send_cap},
    .p_output = {&jpeg::dma_write_SINK},  
    .pi_w = {jpeg::take_1_token(),jpeg::take_1_token()},
    .po_w = {jpeg::call_put_mem(&jpeg::dma_write_mem_put_buf, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = jpeg::con_delay_ns(0)
};

Transition t2 = {
    .id = "t2",
    .delay_f = jpeg::constant_func(66),
    .p_input = {&jpeg::p0,&jpeg::p20,&jpeg::p6},
    .p_output = {&jpeg::p1,&jpeg::p21,&jpeg::p4},  
    .pi_w = {jpeg::con_edge(1),jpeg::con_edge(1),jpeg::con_edge(0)},
    .po_w = {jpeg::con_tokens(1),jpeg::con_tokens(1),jpeg::con_tokens(1)},
    .pi_w_threshold = {jpeg::const_threshold(0), jpeg::const_threshold(0), jpeg::const_threshold(2)},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t3 = {
    .id = "t3",
    .delay_f = jpeg::constant_func(66),
    .p_input = {&jpeg::p0,&jpeg::p21,&jpeg::p6},
    .p_output = {&jpeg::p2,&jpeg::p22,&jpeg::p4},  
    .pi_w = {jpeg::con_edge(1),jpeg::con_edge(4),jpeg::con_edge(0)},
    .po_w = {jpeg::con_tokens(4),jpeg::con_tokens(1),jpeg::con_tokens(1)},
    .pi_w_threshold = {jpeg::const_threshold(0), jpeg::const_threshold(0), jpeg::const_threshold(2)},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t4 = {
    .id = "t4",
    .delay_f = jpeg::constant_func(66),
    .p_input = {&jpeg::p0,&jpeg::p22,&jpeg::p6},
    .p_output = {&jpeg::p3,&jpeg::p20,&jpeg::p4},  
    .pi_w = {jpeg::con_edge(1),jpeg::con_edge(1),jpeg::con_edge(4)},
    .po_w = {jpeg::con_tokens(4),jpeg::con_tokens(4),jpeg::con_tokens(1)},
    .pi_w_threshold = {jpeg::const_threshold(0), jpeg::const_threshold(0), jpeg::const_threshold(2)},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t5 = {
    .id = "t5",
    .delay_f = jpeg::constant_func(65),
    .p_input = {&jpeg::p1,&jpeg::p2,&jpeg::p3},
    .p_output = {&jpeg::pdone,&jpeg::p6},  
    .pi_w = {jpeg::con_edge(1),jpeg::con_edge(1),jpeg::con_edge(1)},
    .po_w = {jpeg::con_tokens(1),jpeg::con_tokens(1),jpeg::con_tokens(1)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t0 = {
    .id = "t0",
    .delay_f = jpeg::mcu_delay(&jpeg::pvarlatency),
    .p_input = {&jpeg::p7,&jpeg::p4,&jpeg::pvarlatency,&jpeg::p_recv_buf},
    .p_output = {&jpeg::p0,&jpeg::p8},  
    .pi_w = {jpeg::con_edge(1),jpeg::con_edge(1),jpeg::con_edge(1), jpeg::special_edge(&jpeg::pvar_recv_token)},
    .po_w = {jpeg::con_tokens(1),jpeg::con_tokens(1)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition t1 = {
    .id = "t1",
    .delay_f = jpeg::constant_func(0),
    .p_input = {&jpeg::ptasks_aft_header,&jpeg::p8},
    .p_output = {&jpeg::p7,&jpeg::pstart},  
    .pi_w = {jpeg::con_edge(1),jpeg::con_edge(1)},
    .po_w = {jpeg::con_tokens(1),jpeg::con_tokens(1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition twait_header = {
    .id = "twait_header",
    .delay_f = jpeg::constant_func(0),
    .p_input = {&jpeg::ptasks_bef_header,&jpeg::p_recv_buf},
    .p_output = {&jpeg::ptasks_aft_header},  
    .pi_w = {jpeg::take_all(&jpeg::ptasks_bef_header), jpeg::con_edge(576)},
    .po_w = {jpeg::pass_all_tokens(&jpeg::ptasks_bef_header)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};

Transition tinp_fifo = {
    .id = "tinp_fifo",
    .delay_f = jpeg::constant_func(8),
    .p_input = {&jpeg::p_req_mem},
    .p_output = {&jpeg::p_send_dma,&jpeg::dma_read_fifo_order,&jpeg::dma_read_req_put_0},  
    .pi_w = {jpeg::take_max_16_tokens(&jpeg::p_req_mem)},
    .po_w = {jpeg::record_max_16(&jpeg::p_req_mem), jpeg::push_request_order(0, &jpeg::p_req_mem), jpeg::mem_request(0, (int)jpeg::CstStr::DMA_READ, &jpeg::p_req_mem)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition toutput_fifo = {
    .id = "toutput_fifo",
    .delay_f = jpeg::constant_func(64),
    .p_input = {&jpeg::pdone},
    .p_output = {&jpeg::dma_write_fifo_order,&jpeg::dma_write_req_put_0},  
    .pi_w = {jpeg::con_edge(1)},
    .po_w = {jpeg::push_request_order_write(0, 32), jpeg::mem_request_write(0, (int)jpeg::CstStr::DMA_WRITE, 32)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition t_input_fifo_recv = {
    .id = "t_input_fifo_recv",
    .delay_f = jpeg::constant_func(0),
    .p_input = {&jpeg::p_send_dma,&jpeg::dma_read_req_get_0},
    .p_output = {&jpeg::p_recv_buf},  
    .pi_w = {jpeg::take_1_token(), jpeg::take_resp_token(&jpeg::p_send_dma)},
    .po_w = {jpeg::put_recv_buf(&jpeg::p_send_dma)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition toutput_fifo_recv = {
    .id = "toutput_fifo_recv",
    .delay_f = jpeg::constant_func(0),
    .p_input = {&jpeg::dma_write_req_get_0},
    .p_output = {},  
    .pi_w = {jpeg::take_resp_token_write(32)},
    .po_w = {},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
}