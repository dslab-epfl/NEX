#pragma once
#include "place_transition.hh"
#include <bits/stdint-uintn.h>

CREATE_TOKEN_TYPE(
token_class_ddopst,
uint64_t dataref;
int dst;
int offset;
int pkt_size;
int requested_size;
int src;
int type;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("dataref")=(int)dataref;
    dict->operator[]("dst")=dst;
    dict->operator[]("offset")=offset;
    dict->operator[]("pkt_size")=pkt_size;
    dict->operator[]("src")=src;
    dict->operator[]("type")=type; 
    return dict; 
})

CREATE_TOKEN_TYPE(
token_class_ddoppst,
uint64_t dataref;
int dst;
int offset;
int pkt_size;
int requested_size;
int port;
int src;
int type;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("dataref")=(int)dataref;
    dict->operator[]("dst")=dst;
    dict->operator[]("offset")=offset;
    dict->operator[]("pkt_size")=pkt_size;
    dict->operator[]("port")=port;
    dict->operator[]("src")=src;
    dict->operator[]("type")=type; 
    return dict; 
})


CREATE_TOKEN_TYPE(
token_class_idx,
int idx;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("idx")=idx; 
    return dict; 
})


CREATE_TOKEN_TYPE(
token_class_ralmtra,
uint64_t req_id;
int pid_fd;
uint64_t addr;
uint64_t len;
uint64_t mem_buffer_ptr;
int type;
uint64_t ref_ptr;
uint64_t total_len;
uint64_t subreq_cnt;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("req_id")=(int)req_id;
    dict->operator[]("pid_fd")=(int)pid_fd;
    dict->operator[]("addr")=(int)addr;
    dict->operator[]("len")=(int)len;
    dict->operator[]("mem_buffer_ptr")=(int)mem_buffer_ptr;
    dict->operator[]("type")=(int)type;
    dict->operator[]("ref_ptr")=(int)ref_ptr; 
    return dict; 
})


CREATE_TOKEN_TYPE(
token_class_abirrs,
uint64_t addr;
uint64_t buffer;
uint64_t id;
int ref;
int rw;
uint64_t size;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("addr")=(int)addr;
    dict->operator[]("buffer")=(int)buffer;
    dict->operator[]("id")=(int)id;
    dict->operator[]("ref")=(int)ref;
    dict->operator[]("rw")=(int)rw;
    dict->operator[]("size")=(int)size; 
    return dict; 
})
