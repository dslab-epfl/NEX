
#pragma once
#include "place_transition.hh"
CREATE_TOKEN_TYPE(
token_class_total_insn,
int total_insn;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("total_insn")=total_insn; 
    return dict; 
})

CREATE_TOKEN_TYPE(
token_class_insn_count,
int insn_count;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("insn_count")=insn_count; 
    return dict; 
})

CREATE_TOKEN_TYPE(
token_class_ostxyuullupppp,
int opcode;
int subopcode;
int tstype;
int xsize;
int ysize;
int uop_begin;
int uop_end;
int lp_1;
int lp_0;
int use_alu_imm;
int pop_prev;
int pop_next;
int push_prev;
int push_next;
std::map<std::string, int>* asDictionary() override{
    std::map<std::string, int>* dict = new std::map<std::string, int>;
    dict->operator[]("opcode")=opcode;
    dict->operator[]("subopcode")=subopcode;
    dict->operator[]("tstype")=tstype;
    dict->operator[]("xsize")=xsize;
    dict->operator[]("ysize")=ysize;
    dict->operator[]("uop_begin")=uop_begin;
    dict->operator[]("uop_end")=uop_end;
    dict->operator[]("lp_1")=lp_1;
    dict->operator[]("lp_0")=lp_0;
    dict->operator[]("use_alu_imm")=use_alu_imm;
    dict->operator[]("pop_prev")=pop_prev;
    dict->operator[]("pop_next")=pop_next;
    dict->operator[]("push_prev")=push_prev;
    dict->operator[]("push_next")=push_next; 
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
