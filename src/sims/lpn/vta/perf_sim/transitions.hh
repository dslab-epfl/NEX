#pragma once
#include "lpn_funcs.hh"

std::vector<Place<token_class_iasbrr>*> list_0 = {&dma_read_req_put_0, &dma_read_req_put_1, &dma_read_req_put_2};
std::vector<Place<token_class_iasbrr>*> list_1 = {&dma_write_req_put_0};
Transition dma_read_arbiter = {
    .id = "dma_read_arbiter",
    .delay_f = con_delay(0),
    .p_input = {&dma_read_fifo_order,&dma_read_req_put_0,&dma_read_req_put_1,&dma_read_req_put_2},
    .p_output = {&dma_read_mem_put_buf},  
    .pi_w = {take_1_token(),arbiterhelperord_take_0_or_1(0, &dma_read_fifo_order),arbiterhelperord_take_0_or_1(1, &dma_read_fifo_order),arbiterhelperord_take_0_or_1(2, &dma_read_fifo_order)},
    .po_w = {arbiterhelperord_pass_turn_token(&dma_read_fifo_order, list_0)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition dma_read_mem_get = {
    .id = "dma_read_mem_get",
    .delay_f = simple_axi_delay(&dma_read_recv_buf),
    .p_input = {&dma_read_recv_buf},
    .p_output = {&dma_read_send_cap,&dma_read_req_get_0,&dma_read_req_get_1,&dma_read_req_get_2},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(),pass_token_match_port(&dma_read_recv_buf, 0),pass_token_match_port(&dma_read_recv_buf, 1),pass_token_match_port(&dma_read_recv_buf, 2)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = {NULL}
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
    .pip = con_delay_ns(0)
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
    .pip = con_delay(0)
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
    .delay_f = simple_axi_delay(&dma_write_recv_buf),
    .p_input = {&dma_write_recv_buf},
    .p_output = {&dma_write_send_cap,&dma_write_req_get_0},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(),pass_token_match_port(&dma_write_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = {NULL}
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
    .pip = con_delay_ns(0)
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

Transition t13 = {
    .id = "t13",
    .delay_f = con_delay(0),
    .p_input = {&plaunch},
    .p_output = {&psReadCmd},  
    .pi_w = {take_1_token()},
    .po_w = {output_insn_read_cmd(&plaunch)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition tload_insn_pre = {
    .id = "tload_insn_pre",
    .delay_f = con_delay(0),
    .p_input = {&psReadCmd,&pcontrol},
    .p_output = {&load_insn_cmd,&dma_read_req_put_0,&dma_read_fifo_order},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {pass_token(&psReadCmd, 1),mem_request(0, (int)vta::CstStr::DMA_LOAD_INSN, 1),push_request_order(0, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition tload_insn_post = {
    .id = "tload_insn_post",
    .delay_f = con_delay(0),
    .p_input = {&load_insn_cmd,&pnumInsn,&dma_read_req_get_0},
    .p_output = {&psDrain,&pcontrol_prime},  
    .pi_w = {take_1_token(),take_readLen(&load_insn_cmd),take_1_token()},
    .po_w = {pass_var_token_readLen(&pnumInsn, &load_insn_cmd),pass_empty_token()},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition t12 = {
    .id = "t12",
    .delay_f = con_delay(1),
    .p_input = {&pcontrol_prime},
    .p_output = {&pcontrol},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token()},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition t14 = {
    .id = "t14",
    .delay_f = con_delay(1),
    .p_input = {&psDrain,&pload_cap},
    .p_output = {&pload_inst_q},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {pass_token(&psDrain, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {take_opcode_token(&psDrain, (int)vta::CstStr::LOAD), empty_guard()},
    .pip = NULL
};
Transition t15 = {
    .id = "t15",
    .delay_f = con_delay(1),
    .p_input = {&psDrain,&pcompute_cap},
    .p_output = {&pcompute_inst_q},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {pass_token(&psDrain, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {take_opcode_token(&psDrain, (int)vta::CstStr::COMPUTE), empty_guard()},
    .pip = NULL
};
Transition t16 = {
    .id = "t16",
    .delay_f = con_delay(1),
    .p_input = {&psDrain,&pstore_cap},
    .p_output = {&pstore_inst_q},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {pass_token(&psDrain, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {take_opcode_token(&psDrain, (int)vta::CstStr::STORE), empty_guard()},
    .pip = NULL
};
Transition tload_launch = {
    .id = "tload_launch",
    .delay_f = con_delay(0),
    .p_input = {&pload_inst_q,&pcompute2load, &pload_ctrl},
    .p_output = {&pload_process,&dma_read_req_put_1,&dma_read_fifo_order},  
    .pi_w = {take_1_token(),take_dep_pop_next(&pload_inst_q),take_1_token()},
    .po_w = {pass_token(&pload_inst_q, 1),mem_request_load_module(1, &pload_inst_q),push_request_order_load_module(1, &pload_inst_q)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition load_done = {
    .id = "load_done",
    .delay_f = delay_load(&pload_process),
    .p_input = {&pload_process,&dma_read_req_get_1},
    .p_output = {&pload_done,&pload2compute,&pload_cap, &pload_ctrl},  
    .pi_w = {take_1_token(),take_resp_token_load_module(&pload_process)},
    .po_w = {pass_empty_token(),output_dep_push_next(&pload_process),pass_empty_token(),pass_empty_token()},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition store_launch = {
    .id = "store_launch",
    .delay_f = con_delay(0),
    .p_input = {&pstore_inst_q,&pcompute2store, &pstore_ctrl},
    .p_output = {&pstore_process,&dma_write_req_put_0,&dma_write_fifo_order},  
    .pi_w = {take_1_token(),take_dep_pop_prev(&pstore_inst_q), take_1_token()},
    .po_w = {pass_token(&pstore_inst_q, 1),mem_request_store_module(0, &pstore_inst_q),push_request_order_store_module(0, &pstore_inst_q)},
    .pi_w_threshold = {NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL},
    .pip = NULL
};
Transition store_done = {
    .id = "store_done",
    .delay_f = delay_store(&pstore_process),
    .p_input = {&pstore_process,&dma_write_req_get_0},
    .p_output = {&pstore_done,&pstore2compute,&pstore_cap, &pstore_ctrl},  
    .pi_w = {take_1_token(),take_resp_token_store_module(&pstore_process)},
    .po_w = {pass_empty_token(),output_dep_push_prev(&pstore_process),pass_empty_token(), pass_empty_token()},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition compute_launch = {
    .id = "compute_launch",
    .delay_f = con_delay(0),
    .p_input = {&pcompute_inst_q,&pstore2compute,&pload2compute, &pcompute_ctrl},
    .p_output = {&pcompute_process,&dma_read_req_put_2,&dma_read_fifo_order},  
    .pi_w = {take_1_token(),take_dep_pop_next(&pcompute_inst_q),take_dep_pop_prev(&pcompute_inst_q), take_1_token()},
    .po_w = {pass_token(&pcompute_inst_q, 1),mem_request_compute_module(2, &pcompute_inst_q),push_request_order_compute_module(2, &pcompute_inst_q)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition compute_done = {
    .id = "compute_done",
    .delay_f = delay_compute(&pcompute_process),
    .p_input = {&pcompute_process,&dma_read_req_get_2},
    .p_output = {&pcompute_done,&pcompute2load,&pcompute2store,&pcompute_cap, &pcompute_ctrl},  
    .pi_w = {take_1_token(),take_resp_token_compute_module(&pcompute_process)},
    .po_w = {pass_empty_token(),output_dep_push_prev(&pcompute_process),output_dep_push_next(&pcompute_process),pass_empty_token(), pass_empty_token()},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};