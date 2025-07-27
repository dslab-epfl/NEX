#pragma once
#include "lpn_funcs.hh"
#include <config/config.h>

#if CONFIG_PCIE_LPN
    #define PCIE_LATENCY 0    
#else
    #define PCIE_LATENCY CONFIG_VTA_LINK_DELAY    
#endif

namespace vta{
std::vector<Place<token_class_iasbrr>*> list_0 = {&vta::dma_read_req_put_0, &vta::dma_read_req_put_1, &vta::dma_read_req_put_2};
std::vector<Place<token_class_iasbrr>*> list_1 = {&vta::dma_write_req_put_0};
Transition dma_read_arbiter = {
    .id = "dma_read_arbiter",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::dma_read_fifo_order,&vta::dma_read_req_put_0,&vta::dma_read_req_put_1,&vta::dma_read_req_put_2},
    .p_output = {&vta::dma_read_mem_put_buf},  
    .pi_w = {vta::take_1_token(),vta::arbiterhelperord_take_0_or_1(0, &vta::dma_read_fifo_order),vta::arbiterhelperord_take_0_or_1(1, &vta::dma_read_fifo_order),vta::arbiterhelperord_take_0_or_1(2, &vta::dma_read_fifo_order)},
    .po_w = {vta::arbiterhelperord_pass_turn_token(&vta::dma_read_fifo_order, vta::list_0)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition dma_read_mem_get = {
    .id = "dma_read_mem_get",
    .delay_f = vta::simple_axi_delay(&vta::dma_read_recv_buf),
    .p_input = {&vta::dma_read_recv_buf},
    .p_output = {&vta::dma_read_send_cap,&vta::dma_read_req_get_0,&vta::dma_read_req_get_1,&vta::dma_read_req_get_2},  
    .pi_w = {vta::take_1_token()},
    .po_w = {vta::pass_empty_token(),vta::pass_token_match_port(&vta::dma_read_recv_buf, 0),vta::pass_token_match_port(&vta::dma_read_recv_buf, 1),vta::pass_token_match_port(&vta::dma_read_recv_buf, 2)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = {NULL}
};
Transition dma_read_recv_from_mem = {
    .id = "dma_read_recv_from_mem",
    .delay_f = vta::delay_latency_if_resp_ready(0, PCIE_LATENCY),
    .p_input = {},
    .p_output = {&vta::dma_read_recv_buf},  
    .pi_w = {},
    .po_w = {vta::call_get_mem(0)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = vta::con_delay_ns(0)
};
Transition dma_read_mem_put = {
    .id = "dma_read_mem_put",
    .delay_f = vta::con_delay_ns(PCIE_LATENCY),
    .p_input = {&vta::dma_read_mem_put_buf,&vta::dma_read_send_cap},
    .p_output = {&vta::dma_read_SINK},  
    .pi_w = {vta::take_1_token(),vta::take_1_token()},
    .po_w = {vta::call_put_mem(&vta::dma_read_mem_put_buf, 0)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = vta::con_delay(0)
};
Transition dma_write_arbiter = {
    .id = "dma_write_arbiter",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::dma_write_fifo_order,&vta::dma_write_req_put_0},
    .p_output = {&vta::dma_write_mem_put_buf},  
    .pi_w = {vta::take_1_token(),vta::arbiterhelperord_take_0_or_1(0, &vta::dma_write_fifo_order)},
    .po_w = {vta::arbiterhelperord_pass_turn_token(&vta::dma_write_fifo_order, vta::list_1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition dma_write_mem_get = {
    .id = "dma_write_mem_get",
    .delay_f = vta::simple_axi_delay(&vta::dma_write_recv_buf),
    .p_input = {&vta::dma_write_recv_buf},
    .p_output = {&vta::dma_write_send_cap,&vta::dma_write_req_get_0},  
    .pi_w = {vta::take_1_token()},
    .po_w = {vta::pass_empty_token(),vta::pass_token_match_port(&vta::dma_write_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = {NULL}
};

Transition dma_write_recv_from_mem = {
    .id = "dma_write_recv_from_mem",
    .delay_f = vta::delay_latency_if_resp_ready(1, PCIE_LATENCY),
    .p_input = {},
    .p_output = {&vta::dma_write_recv_buf},  
    .pi_w = {},
    .po_w = {vta::call_get_mem(1)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = vta::con_delay_ns(0)
};

Transition dma_write_mem_put = {
    .id = "dma_write_mem_put",
    .delay_f = vta::con_delay_ns(PCIE_LATENCY),
    .p_input = {&vta::dma_write_mem_put_buf,&vta::dma_write_send_cap},
    .p_output = {&vta::dma_write_SINK},  
    .pi_w = {vta::take_1_token(),vta::take_1_token()},
    .po_w = {vta::call_put_mem(&vta::dma_write_mem_put_buf, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = con_delay_ns(0)
};

Transition t13 = {
    .id = "t13",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::plaunch},
    .p_output = {&vta::psReadCmd},  
    .pi_w = {vta::take_1_token()},
    .po_w = {vta::output_insn_read_cmd(&vta::plaunch)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition tload_insn_pre = {
    .id = "tload_insn_pre",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::psReadCmd,&vta::pcontrol},
    .p_output = {&vta::load_insn_cmd,&vta::dma_read_req_put_0,&vta::dma_read_fifo_order},  
    .pi_w = {vta::take_1_token(),vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::psReadCmd, 1),vta::mem_request(0, (int)vta::CstStr::DMA_LOAD_INSN, 1),vta::push_request_order(0, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {vta::psDrain_is_empty(), vta::empty_guard()},
    .pip = NULL
};
Transition tload_insn_post = {
    .id = "tload_insn_post",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::load_insn_cmd,&vta::pnumInsn,&vta::dma_read_req_get_0},
    .p_output = {&vta::psDrain,&vta::pcontrol_prime},  
    .pi_w = {vta::take_1_token(),vta::take_readLen(&vta::load_insn_cmd),vta::take_1_token()},
    .po_w = {vta::pass_var_token_readLen(&vta::pnumInsn, &vta::load_insn_cmd),vta::pass_empty_token()},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t12 = {
    .id = "t12",
    .delay_f = vta::con_delay(1),
    .p_input = {&vta::pcontrol_prime},
    .p_output = {&vta::pcontrol},  
    .pi_w = {vta::take_1_token()},
    .po_w = {vta::pass_empty_token()},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition t14 = {
    .id = "t14",
    .delay_f = vta::con_delay(1),
    .p_input = {&vta::psDrain,&vta::pload_cap},
    .p_output = {&vta::pload_inst_q},  
    .pi_w = {vta::take_1_token(),vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::psDrain, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {vta::take_opcode_token(&vta::psDrain, (int)vta::CstStr::LOAD), vta::empty_guard()},
    .pip = NULL
};
Transition t15 = {
    .id = "t15",
    .delay_f = vta::con_delay(1),
    .p_input = {&vta::psDrain,&vta::pcompute_cap},
    .p_output = {&vta::pcompute_inst_q},  
    .pi_w = {vta::take_1_token(),vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::psDrain, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {vta::take_opcode_token(&vta::psDrain, (int)vta::CstStr::COMPUTE), vta::empty_guard()},
    .pip = NULL
};
Transition t16 = {
    .id = "t16",
    .delay_f = vta::con_delay(1),
    .p_input = {&vta::psDrain,&vta::pstore_cap},
    .p_output = {&vta::pstore_inst_q},  
    .pi_w = {vta::take_1_token(),vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::psDrain, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {vta::take_opcode_token(&vta::psDrain, (int)vta::CstStr::STORE), vta::empty_guard()},
    .pip = NULL
};
Transition tload_launch = {
    .id = "tload_launch",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::pload_inst_q,&vta::pcompute2load, &vta::pload_ctrl},
    .p_output = {&vta::pload_process,&vta::dma_read_req_put_1,&vta::dma_read_fifo_order},  
    .pi_w = {vta::take_1_token(),vta::take_dep_pop_next(&vta::pload_inst_q),vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::pload_inst_q, 1),vta::mem_request_load_module(1, &vta::pload_inst_q),vta::push_request_order_load_module(1, &vta::pload_inst_q)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition load_done = {
    .id = "load_done",
    .delay_f = vta::delay_load(&vta::pload_process),
    .p_input = {&vta::pload_process,&vta::dma_read_req_get_1},
    .p_output = {&vta::pload_done,&vta::pload2compute,&vta::pload_cap, &vta::pload_ctrl},  
    .pi_w = {vta::take_1_token(),vta::take_resp_token_load_module(&vta::pload_process)},
    .po_w = {vta::pass_empty_token(),vta::output_dep_push_next(&vta::pload_process),vta::pass_empty_token(),vta::pass_empty_token()},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition store_launch = {
    .id = "store_launch",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::pstore_inst_q,&vta::pcompute2store, &vta::pstore_ctrl},
    .p_output = {&vta::pstore_process,&vta::dma_write_req_put_0,&vta::dma_write_fifo_order},  
    .pi_w = {vta::take_1_token(),vta::take_dep_pop_prev(&vta::pstore_inst_q), vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::pstore_inst_q, 1),vta::mem_request_store_module(0, &vta::pstore_inst_q),vta::push_request_order_store_module(0, &vta::pstore_inst_q)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition store_done = {
    .id = "store_done",
    .delay_f = vta::delay_store(&vta::pstore_process),
    .p_input = {&vta::pstore_process,&vta::dma_write_req_get_0},
    .p_output = {&vta::pstore_done,&vta::pstore2compute,&vta::pstore_cap, &vta::pstore_ctrl},  
    .pi_w = {vta::take_1_token(),vta::take_resp_token_store_module(&vta::pstore_process)},
    .po_w = {vta::pass_empty_token(),vta::output_dep_push_prev(&vta::pstore_process),vta::pass_empty_token(), vta::pass_empty_token()},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition compute_launch = {
    .id = "compute_launch",
    .delay_f = vta::con_delay(0),
    .p_input = {&vta::pcompute_inst_q,&vta::pstore2compute,&vta::pload2compute, &vta::pcompute_ctrl},
    .p_output = {&vta::pcompute_process,&vta::dma_read_req_put_2,&vta::dma_read_fifo_order},  
    .pi_w = {vta::take_1_token(),vta::take_dep_pop_next(&vta::pcompute_inst_q),vta::take_dep_pop_prev(&vta::pcompute_inst_q), vta::take_1_token()},
    .po_w = {vta::pass_token(&vta::pcompute_inst_q, 1),vta::mem_request_compute_module(2, &vta::pcompute_inst_q),vta::push_request_order_compute_module(2, &vta::pcompute_inst_q)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition compute_done = {
    .id = "compute_done",
    .delay_f = vta::delay_compute(&vta::pcompute_process),
    .p_input = {&vta::pcompute_process,&vta::dma_read_req_get_2},
    .p_output = {&vta::pcompute_done,&vta::pcompute2load,&vta::pcompute2store,&vta::pcompute_cap, &vta::pcompute_ctrl},  
    .pi_w = {vta::take_1_token(),vta::take_resp_token_compute_module(&vta::pcompute_process)},
    .po_w = {vta::pass_empty_token(),vta::output_dep_push_prev(&vta::pcompute_process),vta::output_dep_push_next(&vta::pcompute_process),vta::pass_empty_token(), vta::pass_empty_token()},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
}