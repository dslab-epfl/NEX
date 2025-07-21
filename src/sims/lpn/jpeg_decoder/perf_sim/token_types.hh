
#pragma once
#include "sims/lpn/lpn_common/place_transition.hh"

CREATE_TOKEN_TYPE(
token_class_avg_block_size,
int avg_block_size;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("avg_block_size")=avg_block_size; 
    return dict; 
})


CREATE_TOKEN_TYPE(
token_class_iasbrr,
int id;
int addr;
int size;
int buffer;
int rw;
int ref;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("id")=id;
    dict->operator[]("addr")=addr;
    dict->operator[]("size")=size;
    dict->operator[]("buffer")=buffer;
    dict->operator[]("rw")=rw;
    dict->operator[]("ref")=ref; 
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
token_class_nonzero,
int nonzero;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("nonzero")=nonzero; 
    return dict; 
})

CREATE_TOKEN_TYPE(
token_class_req_len,
int req_len;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("req_len")=req_len; 
    return dict; 
})

CREATE_TOKEN_TYPE(
token_class_num,
int num;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("num")=num; 
    return dict; 
})
