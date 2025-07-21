#pragma once
#include <stdlib.h>
#include <functional>
#include "place_transition.hh"
#include "places.hh"
#include "lpn_funcs.hh"
#include "config.hh"

std::vector<Place<token_class_ddopst>*> list_0 = {&root_tmpbuf_1_0};
std::vector<Place<token_class_ddopst>*> list_1 = {&root_tmpbuf_1_0};
std::vector<Place<token_class_ddopst>*> list_2 = {&root_tmpbuf_1_0};
std::vector<int> list_3 = {1};
std::vector<Place<token_class_ddopst>*> list_4 = {&root_tmpbuf_0_1};
std::vector<Place<token_class_ddopst>*> list_5 = {&root_tmpbuf_0_1};
std::vector<Place<token_class_ddopst>*> list_6 = {&root_tmpbuf_0_1};
std::vector<int> list_7 = {0};
std::vector<Place<token_class_ddopst>*> list_8 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_9 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_10 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_11 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_12 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_13 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_14 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_15 = {&d1_reqbuf_nonpost, &d1_reqbuf_post, &d1_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_16 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_17 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_18 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_19 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_20 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_21 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_22 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::vector<Place<token_class_ddopst>*> list_23 = {&d0_reqbuf_nonpost, &d0_reqbuf_post, &d0_reqbuf_cmpl};
std::map<int, int> dict_0 = {
{0, 0}, 
{1, 1}, 
};
std::map<int, int> dict_1 = {
{0, 0}, 
{1, 1}, 
};
Transition troot_tmpbuf_0_1 = {
    .id = "troot_tmpbuf_0_1",
    .delay_f = con_delay_ns(1),
    .p_input = {&root_recv_0,&d1_recvcap},
    .p_output = {&root_tmpbuf_0_1},  
    .pi_w = {con_edge(1),take_some_token(0)},
    .po_w = {pass_atomic_smaller_token(MPS, &root_recv_0)},
    .pi_w_threshold = {empty_threshold(), take_credit_token(&root_recv_0, UNIT)},
    .pi_guard = {take_out_port(&root_recv_0, dict_0, 1), empty_guard()},
    .pip = NULL
};
Transition troot_tmpbuf_1_0 = {
    .id = "troot_tmpbuf_1_0",
    .delay_f = con_delay_ns(1),
    .p_input = {&root_recv_1,&d0_recvcap},
    .p_output = {&root_tmpbuf_1_0},  
    .pi_w = {con_edge(1),take_some_token(0)},
    .po_w = {pass_atomic_smaller_token(MPS, &root_recv_1)},
    .pi_w_threshold = {empty_threshold(), take_credit_token(&root_recv_1, UNIT)},
    .pi_guard = {take_out_port(&root_recv_1, dict_1, 0), empty_guard()},
    .pip = NULL
};
Transition root_tmpbuf_to_sendbuf_0_arbiter = {
    .id = "root_tmpbuf_to_sendbuf_0_arbiter",
    .delay_f = con_delay_ns(0),
    .p_input = {&root_tmpbuf_to_sendbuf_0_pidx,&root_tmpbuf_1_0},
    .p_output = {&root_tmpbuf_to_sendbuf_0_pidx,&root_sendbuf_0},  
    .pi_w = {take_1_token(),arbiterhelper_take_0_or_1(0, list_0, &root_tmpbuf_to_sendbuf_0_pidx)},
    .po_w = {arbiterhelper_update_cur_turn(&root_tmpbuf_to_sendbuf_0_pidx, list_1),arbiterhelpernocapwport_pass_turn_token(&root_tmpbuf_to_sendbuf_0_pidx, list_2, list_3)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition root_send_0 = {
    .id = "root_send_0",
    .delay_f = var_tlp_process_delay(&root_sendbuf_0, ProcessTLPCost),
    .p_input = {&root_sendbuf_0,&d0_recvcap},
    .p_output = {&root_recvcap_1,&root_P2P_d0_linkbuf,&root_P2P_d0_tlp},  
    .pi_w = {take_1_token(),take_credit_token(&root_sendbuf_0, UNIT)},
    .po_w = {increase_credit(&root_sendbuf_0, 1, UNIT),pass_credit_with_header_token(&root_sendbuf_0, 30, UNIT),pass_add_header_token(&root_sendbuf_0, 1, 30)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition root_tmpbuf_to_sendbuf_1_arbiter = {
    .id = "root_tmpbuf_to_sendbuf_1_arbiter",
    .delay_f = con_delay_ns(0),
    .p_input = {&root_tmpbuf_to_sendbuf_1_pidx,&root_tmpbuf_0_1},
    .p_output = {&root_tmpbuf_to_sendbuf_1_pidx,&root_sendbuf_1},  
    .pi_w = {take_1_token(),arbiterhelper_take_0_or_1(0, list_4, &root_tmpbuf_to_sendbuf_1_pidx)},
    .po_w = {arbiterhelper_update_cur_turn(&root_tmpbuf_to_sendbuf_1_pidx, list_5),arbiterhelpernocapwport_pass_turn_token(&root_tmpbuf_to_sendbuf_1_pidx, list_6, list_7)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition root_send_1 = {
    .id = "root_send_1",
    .delay_f = var_tlp_process_delay(&root_sendbuf_1, ProcessTLPCost),
    .p_input = {&root_sendbuf_1,&d1_recvcap},
    .p_output = {&root_recvcap_0,&root_P2P_d1_linkbuf,&root_P2P_d1_tlp},  
    .pi_w = {take_1_token(),take_credit_token(&root_sendbuf_1, UNIT)},
    .po_w = {increase_credit(&root_sendbuf_1, 0, UNIT),pass_credit_with_header_token(&root_sendbuf_1, 30, UNIT),pass_add_header_token(&root_sendbuf_1, 1, 30)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition d1_send = {
    .id = "d1_send",
    .delay_f = var_tlp_process_delay(&d1_reqbuf, ProcessTLPCost),
    .p_input = {&d1_reqbuf,&root_recvcap_1},
    .p_output = {&d1_reqbuf_cap,&d1_P2P_root_tlp,&d1_P2P_root_linkbuf},  
    .pi_w = {take_1_token(),take_credit_token(&d1_reqbuf, UNIT)},
    .po_w = {pass_empty_token(1),pass_add_header_token(&d1_reqbuf, 1, 30),pass_credit_with_header_token(&d1_reqbuf, 30, UNIT)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition d1_proc = {
    .id = "d1_proc",
    .delay_f = con_delay_ns(0),
    .p_input = {&d1_recv},
    .p_output = {&d1_recvcmpl,&d1_recvreq,&d1_recvcap},  
    .pi_w = {take_1_token()},
    .po_w = {store_cmpl(&d1_recv),store_request(&d1_recv),increase_credit_wc(&d1_recv, UNIT)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition d1_arbiter = {
    .id = "d1_arbiter",
    .delay_f = con_delay_ns(0),
    .p_input = {&d1_pidx,&d1_reqbuf_cap,&d1_reqbuf_nonpost,&d1_reqbuf_post,&d1_reqbuf_cmpl},
    .p_output = {&d1_pidx,&d1_reqbuf,&d1_reqbuf_nonpost_cap,&d1_reqbuf_post_cap,&d1_reqbuf_cmpl_cap},  
    .pi_w = {take_1_token(),take_1_token(),arbiterhelper_take_0_or_1(0, list_8, &d1_pidx),arbiterhelper_take_0_or_1(1, list_9, &d1_pidx),arbiterhelper_take_0_or_1(2, list_10, &d1_pidx)},
    .po_w = {arbiterhelper_update_cur_turn(&d1_pidx, list_11),arbiterhelper_pass_turn_token(&d1_pidx, list_12),arbiterhelper_pass_turn_empty_token(&d1_pidx, 0, list_13),arbiterhelper_pass_turn_empty_token(&d1_pidx, 1, list_14),arbiterhelper_pass_turn_empty_token(&d1_pidx, 2, list_15)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition d0_send = {
    .id = "d0_send",
    .delay_f = var_tlp_process_delay(&d0_reqbuf, ProcessTLPCost),
    .p_input = {&d0_reqbuf,&root_recvcap_0},
    .p_output = {&d0_reqbuf_cap,&d0_P2P_root_tlp,&d0_P2P_root_linkbuf},  
    .pi_w = {take_1_token(),take_credit_token(&d0_reqbuf, UNIT)},
    .po_w = {pass_empty_token(1),pass_add_header_token(&d0_reqbuf, 1, 30),pass_credit_with_header_token(&d0_reqbuf, 30, UNIT)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition d0_proc = {
    .id = "d0_proc",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_recv, &d0_mem_req_max_inflight},
    .p_output = {&d0_recvcmpl,&d0_recvreq,&d0_recvcap},  
    .pi_w = {take_1_token(), take_1_if_mem_req(&d0_recv)},
    .po_w = {store_cmpl(&d0_recv),store_request(&d0_recv),increase_credit_wc(&d0_recv, UNIT)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition d0_arbiter = {
    .id = "d0_arbiter",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_pidx,&d0_reqbuf_cap,&d0_reqbuf_nonpost,&d0_reqbuf_post,&d0_reqbuf_cmpl},
    .p_output = {&d0_pidx,&d0_reqbuf,&d0_reqbuf_nonpost_cap,&d0_reqbuf_post_cap,&d0_reqbuf_cmpl_cap},  
    .pi_w = {take_1_token(),take_1_token(),arbiterhelper_take_0_or_1(0, list_16, &d0_pidx),arbiterhelper_take_0_or_1(1, list_17, &d0_pidx),arbiterhelper_take_0_or_1(2, list_18, &d0_pidx)},
    .po_w = {arbiterhelper_update_cur_turn(&d0_pidx, list_19),arbiterhelper_pass_turn_token(&d0_pidx, list_20),arbiterhelper_pass_turn_empty_token(&d0_pidx, 0, list_21),arbiterhelper_pass_turn_empty_token(&d0_pidx, 1, list_22),arbiterhelper_pass_turn_empty_token(&d0_pidx, 2, list_23)},
    .pi_w_threshold = {NULL, NULL, NULL, NULL, NULL},
    .pi_guard = {NULL, NULL, NULL, NULL, NULL},
    .pip = NULL
};
Transition d0_get_read = {
    .id = "d0_get_read",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_mem_read_req_get_0},
    .p_output = {&d0_resp_buf},  
    .pi_w = {take_1_token()},
    .po_w = {pass_token(&d0_mem_read_req_get_0, 1)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition d0_get_write = {
    .id = "d0_get_write",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_mem_write_req_get_0},
    .p_output = {&d0_resp_buf},  
    .pi_w = {take_1_token()},
    .po_w = {pass_token(&d0_mem_write_req_get_0, 1)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition d0_send_mem_req = {
    .id = "d0_send_mem_req",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_recvreq},
    .p_output = {&d0_mem_read_req_put_0,&d0_mem_write_req_put_0},  
    .pi_w = {take_1_token()},
    .po_w = {mem_request_read(&d0_recvreq, MPS, 0),mem_request_write(&d0_recvreq, MPS, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = NULL
};
Transition d0_send_cmpl_reply = {
    .id = "d0_send_cmpl_reply",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_resp_buf,&d0_reqbuf_cmpl_cap},
    .p_output = {&d0_reqbuf_cmpl, &d0_mem_req_max_inflight},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {send_cmpl(&d0_resp_buf), pass_empty_token(1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition d0_mem_read_mem_get = {
    .id = "d0_mem_read_mem_get",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_mem_read_recv_buf},
    .p_output = {&d0_mem_read_send_cap,&d0_mem_read_req_get_0},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(1),pass_token_match_port(&d0_mem_read_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(0)
};
Transition d0_mem_read_recv_from_mem = {
    .id = "d0_mem_read_recv_from_mem",
    .delay_f = delay_0_if_resp_ready(0),
    .p_input = {},
    .p_output = {&d0_mem_read_recv_buf},  
    .pi_w = {},
    .po_w = {call_get_mem(0)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = NULL
};
Transition d0_mem_read_mem_put = {
    .id = "d0_mem_read_mem_put",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_mem_read_req_put_0,&d0_mem_read_send_cap},
    .p_output = {&d0_mem_read_SINK},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {call_put_mem(&d0_mem_read_req_put_0, 0)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = con_delay_ns(0)
};
Transition d0_mem_write_mem_get = {
    .id = "d0_mem_write_mem_get",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_mem_write_recv_buf},
    .p_output = {&d0_mem_write_send_cap,&d0_mem_write_req_get_0},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(1),pass_token_match_port(&d0_mem_write_recv_buf, 0)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(0)
};
Transition d0_mem_write_recv_from_mem = {
    .id = "d0_mem_write_recv_from_mem",
    .delay_f = delay_0_if_resp_ready(1),
    .p_input = {},
    .p_output = {&d0_mem_write_recv_buf},  
    .pi_w = {},
    .po_w = {call_get_mem(1)},
    .pi_w_threshold = {},
    .pi_guard = {},
    .pip = NULL
};
Transition d0_mem_write_mem_put = {
    .id = "d0_mem_write_mem_put",
    .delay_f = con_delay_ns(0),
    .p_input = {&d0_mem_write_req_put_0,&d0_mem_write_send_cap},
    .p_output = {&d0_mem_write_SINK},  
    .pi_w = {take_1_token(),take_1_token()},
    .po_w = {call_put_mem(&d0_mem_write_req_put_0, 1)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = con_delay_ns(0)
};
Transition root_P2P_d0_transfer = {
    .id = "root_P2P_d0_transfer",
    .delay_f = con_delay_ns(DelayCycle),
    .p_input = {&root_P2P_d0_linkbuf},
    .p_output = {&root_P2P_d0_outbuf},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(1)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(1)
};
Transition root_P2P_d0_merge = {
    .id = "root_P2P_d0_merge",
    .delay_f = merge_delay(&root_P2P_d0_tlp),
    .p_input = {&root_P2P_d0_tlp,&root_P2P_d0_outbuf},
    .p_output = {&d0_recv},  
    .pi_w = {take_1_token(),take_credit_token(&root_P2P_d0_tlp, UNIT)},
    .po_w = {pass_rm_header_token(&root_P2P_d0_tlp, 1, 30)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition d0_P2P_root_transfer = {
    .id = "d0_P2P_root_transfer",
    .delay_f = con_delay_ns(DelayCycle),
    .p_input = {&d0_P2P_root_linkbuf},
    .p_output = {&d0_P2P_root_outbuf},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(1)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(1)
};
Transition d0_P2P_root_merge = {
    .id = "d0_P2P_root_merge",
    .delay_f = merge_delay(&d0_P2P_root_tlp),
    .p_input = {&d0_P2P_root_tlp,&d0_P2P_root_outbuf},
    .p_output = {&root_recv_0},  
    .pi_w = {take_1_token(),take_credit_token(&d0_P2P_root_tlp, UNIT)},
    .po_w = {pass_rm_header_token(&d0_P2P_root_tlp, 1, 30)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition root_P2P_d1_transfer = {
    .id = "root_P2P_d1_transfer",
    .delay_f = con_delay_ns(DelayCycle),
    .p_input = {&root_P2P_d1_linkbuf},
    .p_output = {&root_P2P_d1_outbuf},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(1)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(1)
};
Transition root_P2P_d1_merge = {
    .id = "root_P2P_d1_merge",
    .delay_f = merge_delay(&root_P2P_d1_tlp),
    .p_input = {&root_P2P_d1_tlp,&root_P2P_d1_outbuf},
    .p_output = {&d1_recv},  
    .pi_w = {take_1_token(),take_credit_token(&root_P2P_d1_tlp, UNIT)},
    .po_w = {pass_rm_header_token(&root_P2P_d1_tlp, 1, 30)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};
Transition d1_P2P_root_transfer = {
    .id = "d1_P2P_root_transfer",
    .delay_f = con_delay_ns(DelayCycle),
    .p_input = {&d1_P2P_root_linkbuf},
    .p_output = {&d1_P2P_root_outbuf},  
    .pi_w = {take_1_token()},
    .po_w = {pass_empty_token(1)},
    .pi_w_threshold = {NULL},
    .pi_guard = {NULL},
    .pip = con_delay_ns(1)
};
Transition d1_P2P_root_merge = {
    .id = "d1_P2P_root_merge",
    .delay_f = merge_delay(&d1_P2P_root_tlp),
    .p_input = {&d1_P2P_root_tlp,&d1_P2P_root_outbuf},
    .p_output = {&root_recv_1},  
    .pi_w = {take_1_token(),take_credit_token(&d1_P2P_root_tlp, UNIT)},
    .po_w = {pass_rm_header_token(&d1_P2P_root_tlp, 1, 30)},
    .pi_w_threshold = {NULL, NULL},
    .pi_guard = {NULL, NULL},
    .pip = NULL
};