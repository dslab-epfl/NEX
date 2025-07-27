#pragma once

#include "place_transition.hh"
#include <bits/stdint-uintn.h>


CREATE_TOKEN_TYPE(
token_class_num,
int num;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("num")=num; 
    return dict; 
})


CREATE_TOKEN_TYPE(
token_class_addr_type,
uint64_t addr;
int type;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("addr")=addr;
    dict->operator[]("type")=type; 
    return dict; 
})

CREATE_TOKEN_TYPE(
token_class_ralmtrd,
uint64_t req_id;
int dma_fd;
uint64_t addr;
uint64_t len;
uint64_t mem_buffer_ptr;
int type;
uint64_t ref_ptr;
int dev_id;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("req_id")=(int)req_id;
    dict->operator[]("dma_fd")=(int)dma_fd;
    dict->operator[]("addr")=(int)addr;
    dict->operator[]("len")=(int)len;
    dict->operator[]("mem_buffer_ptr")=(int)mem_buffer_ptr;
    dict->operator[]("type")=(int)type;
    dict->operator[]("ref_ptr")=(int)ref_ptr; 
    return dict; 
})
