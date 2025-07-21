#pragma once
#include <stdlib.h>
#include <functional>
#include "places.hh"
#include "lpn_funcs.hh"

std::vector<Place<token_class_iasbrr>*> list_0 = {&dma_read_req_put_0};
std::vector<Place<token_class_iasbrr>*> list_1 = {&dma_write_req_put_0};
Transition dma_read_arbiter = {
    .id = "dma_read_arbiter",
    .delay_f = con_delay(0),
    .p_input = {&dma_read_fifo_order,&dma_read_req_put_0},
    .p_output = {&dma_read_mem_put_buf},  
    .pi_w = {take_1_token(),arbiterhelperord_take_0_or_1(0, &dma_read_fifo_order)},
    .po_w = {arbiterhelperord_pass_turn_token(&dma_read_fifo_order, list_0)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition dma_read_mem_get = {
    .id = "dma_read_mem_get",
    .delay_f = con_delay_ns(0),
    .p_input = {&dma_read_recv_buf},
    .p_output = {&dma_read_send_cap,&dma_read_req_get_0},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(),pass_token_match_port(&dma_read_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(0)
};
Transition dma_read_recv_from_mem = {
    .id = "dma_read_recv_from_mem",
    .delay_f = delay_0_if_resp_ready(0),
    .p_input = {},
    .p_output = {&dma_read_recv_buf},  
    .pi_w = {},
    .po_w = {call_get_mem(0)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = NULL
};
Transition dma_read_mem_put = {
    .id = "dma_read_mem_put",
    .delay_f = con_delay_ns(0),
    .p_input = {&dma_read_mem_put_buf,&dma_read_send_cap},
    .p_output = {&dma_read_SINK},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {call_put_mem(&dma_read_mem_put_buf, 0)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = con_delay_ns(0)
};
Transition dma_write_arbiter = {
    .id = "dma_write_arbiter",
    .delay_f = con_delay(0),
    .p_input = {&dma_write_fifo_order,&dma_write_req_put_0},
    .p_output = {&dma_write_mem_put_buf},  
    .pi_w = {take_1_token(),arbiterhelperord_take_0_or_1(0, &dma_write_fifo_order)},
    .po_w = {arbiterhelperord_pass_turn_token(&dma_write_fifo_order, list_1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition dma_write_mem_get = {
    .id = "dma_write_mem_get",
    .delay_f = con_delay_ns(0),
    .p_input = {&dma_write_recv_buf},
    .p_output = {&dma_write_send_cap,&dma_write_req_get_0},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(),pass_token_match_port(&dma_write_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(0)
};
Transition dma_write_recv_from_mem = {
    .id = "dma_write_recv_from_mem",
    .delay_f = delay_0_if_resp_ready(1),
    .p_input = {},
    .p_output = {&dma_write_recv_buf},  
    .pi_w = {},
    .po_w = {call_get_mem(1)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = NULL
};
Transition dma_write_mem_put = {
    .id = "dma_write_mem_put",
    .delay_f = con_delay_ns(0),
    .p_input = {&dma_write_mem_put_buf,&dma_write_send_cap},
    .p_output = {&dma_write_SINK},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {call_put_mem(&dma_write_mem_put_buf, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = con_delay_ns(0)
};

Transition t2 = {
    .id = "t2",
    .delay_f = constant_func(66),
    .p_input = {&p0,&p20,&p6},
    .p_output = {&p1,&p21,&p4},  
    .pi_w = {con_edge(1),con_edge(1),con_edge(0)},
    .po_w = {con_tokens(1),con_tokens(1),con_tokens(1)},
    .pi_w_threshold = {const_threshold(0), const_threshold(0), const_threshold(2)},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t3 = {
    .id = "t3",
    .delay_f = constant_func(66),
    .p_input = {&p0,&p21,&p6},
    .p_output = {&p2,&p22,&p4},  
    .pi_w = {con_edge(1),con_edge(4),con_edge(0)},
    .po_w = {con_tokens(4),con_tokens(1),con_tokens(1)},
    .pi_w_threshold = {const_threshold(0), const_threshold(0), const_threshold(2)},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t4 = {
    .id = "t4",
    .delay_f = constant_func(66),
    .p_input = {&p0,&p22,&p6},
    .p_output = {&p3,&p20,&p4},  
    .pi_w = {con_edge(1),con_edge(1),con_edge(4)},
    .po_w = {con_tokens(4),con_tokens(4),con_tokens(1)},
    .pi_w_threshold = {const_threshold(0), const_threshold(0), const_threshold(2)},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t5 = {
    .id = "t5",
    .delay_f = constant_func(65),
    .p_input = {&p1,&p2,&p3},
    .p_output = {&pdone,&p6},  
    .pi_w = {con_edge(1),con_edge(1),con_edge(1)},
    .po_w = {con_tokens(1),con_tokens(1),con_tokens(1)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t0 = {
    .id = "t0",
    .delay_f = mcu_delay(&pvarlatency),
    .p_input = {&p7,&p4,&pvarlatency,&p_recv_buf},
    .p_output = {&p0,&p8},  
    .pi_w = {con_edge(1),con_edge(1),con_edge(1), special_edge(&pvar_recv_token)},
    .po_w = {con_tokens(1),con_tokens(1)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition t1 = {
    .id = "t1",
    .delay_f = constant_func(0),
    .p_input = {&ptasks_aft_header,&p8},
    .p_output = {&p7,&pstart},  
    .pi_w = {con_edge(1),con_edge(1)},
    .po_w = {con_tokens(1),con_tokens(1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition twait_header = {
    .id = "twait_header",
    .delay_f = constant_func(0),
    .p_input = {&ptasks_bef_header,&p_recv_buf},
    .p_output = {&ptasks_aft_header},  
    .pi_w = {take_all(&ptasks_bef_header),con_edge(576)},
    .po_w = {pass_all_tokens(&ptasks_bef_header)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};

Transition tinp_fifo = {
    .id = "tinp_fifo",
    .delay_f = constant_func(8),
    .p_input = {&p_req_mem},
    .p_output = {&p_send_dma,&dma_read_fifo_order,&dma_read_req_put_0},  
    .pi_w = {take_max_16_tokens(&p_req_mem)},
    .po_w = {record_max_16(&p_req_mem),push_request_order(0, &p_req_mem),mem_request(0, (int)jpeg::CstStr::DMA_READ, &p_req_mem)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition toutput_fifo = {
    .id = "toutput_fifo",
    .delay_f = constant_func(64),
    .p_input = {&pdone},
    .p_output = {&dma_write_fifo_order,&dma_write_req_put_0},  
    .pi_w = {con_edge(1)},
    .po_w = {push_request_order_write(0, 32),mem_request_write(0, (int)jpeg::CstStr::DMA_WRITE, 32)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition t_input_fifo_recv = {
    .id = "t_input_fifo_recv",
    .delay_f = constant_func(0),
    .p_input = {&p_send_dma,&dma_read_req_get_0},
    .p_output = {&p_recv_buf},  
    .pi_w = {take_1_token(),take_resp_token(&p_send_dma)},
    .po_w = {put_recv_buf(&p_send_dma)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition toutput_fifo_recv = {
    .id = "toutput_fifo_recv",
    .delay_f = constant_func(0),
    .p_input = {&dma_write_req_get_0},
    .p_output = {},  
    .pi_w = {take_resp_token_write(32)},
    .po_w = {},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};