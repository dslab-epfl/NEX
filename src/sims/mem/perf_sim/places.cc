#include "places.hh"
// Place<token_class_ralmtrd> dram_mem_complete("dram_mem_complete");
// Place<token_class_ralmtrd> dram_mem_req("dram_mem_req");
// Place<token_class_ralmtrd> dram_mem_resp("dram_mem_resp");
// Place<> dram_mem_read_cap("dram_mem_read_cap");
// Place<> dram_mem_write_cap("dram_mem_write_cap");

Place<token_class_addr_type> memory_resp_cl("memory_resp_cl");
Place<token_class_ralmtrd> memory_resp("memory_resp");
Place<token_class_ralmtrd> memory_req_front("memory_req_front");
Place<token_class_num> inflight_cl_cnt("inflight_cl_cnt");
Place<token_class_addr_type> memory_req_cl("memory_req_cl");
Place<> memory_inflight_cap("memory_inflight_cap");
Place<token_class_ralmtrd> memory_req_store("memory_req_store");


Place<token_class_ralmtrd> memory_resp_0("memory_resp_0");
Place<token_class_ralmtrd> memory_resp_1("memory_resp_1");
Place<token_class_ralmtrd> memory_resp_2("memory_resp_2");
Place<token_class_ralmtrd> memory_resp_3("memory_resp_3");
Place<token_class_ralmtrd> memory_resp_4("memory_resp_4");
Place<token_class_ralmtrd> memory_resp_5("memory_resp_5");
Place<token_class_ralmtrd> memory_resp_6("memory_resp_6");
Place<token_class_ralmtrd> memory_resp_7("memory_resp_7");
Place<token_class_ralmtrd> memory_resp_8("memory_resp_8");
Place<token_class_ralmtrd> memory_resp_9("memory_resp_9");

Place<> dummy_place("dummy_place");
