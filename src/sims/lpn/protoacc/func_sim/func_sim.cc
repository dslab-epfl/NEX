#include "uint128_t_misc.hh"
#include <bits/stdint-uintn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stack>
#include <fstream>
#include "../include/lpn_req_map.hh"
#include "../include/driver.hh"

// #define DEBUG_PRINT(...) printf(__VA_ARGS__)
#define DEBUG_PRINT(...)

static uint64_t id_counter = 0;
using json = nlohmann::json;

std::stack<json*> message_stack;
json* current_message = nullptr;

void new_message() {
    json* message = new json();
    current_message = message;
    message_stack.push(current_message);
}

void end_message(){
    // Write the message to a file
    json* message = message_stack.top();
    message_stack.pop();
    std::string message_str = message->dump(4);
    // std::cout << message_str << std::endl;
    std::string filename = "/tmp/zurvan_protoacc_output.json";
    std::ofstream file(filename, std::ios_base::out);
    if (file.is_open()) {
        // Write the JSON message to the file
        file << message_str << std::endl;  // dump(4) formats the JSON with 4-space indentation
        file.close();  // Close the file
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }
    // delete message_stack.top();
}

void new_submessage() {
    // Create a new submessage
    current_message =  new json();
    message_stack.push(current_message);
}

void end_submessage(const std::string& field_name, bool is_repeated = false) {
    // Move back to the parent message by popping from the stack
    message_stack.pop();
    json* parent_message = message_stack.top();
    (*parent_message)[field_name]["type"] = "submessage";
    (*parent_message)[field_name]["is_repeated"] = is_repeated;
    (*parent_message)[field_name]["data"] = *current_message;
    current_message = message_stack.top();
}

void add_field(const std::string& field_name, const std::string& field_type, bool is_repeated, int* data, int data_size) {
    // Create a new field
    DEBUG_PRINT("//// add field %s\n", field_name.c_str());
    json field;
    field["type"] = field_type;
    field["is_repeated"] = is_repeated;
    field["data"] = json::array();
    for(int i = 0; i < data_size; i++) {
        field["data"].push_back(data[i]);
    }

    if (current_message != nullptr) {
        // Add the field to the current submessage
        (*current_message)[field_name] = field;
    } else {
        assert(0);
    }
}

ssize_t read_process_memory(uintptr_t address, void *buffer, size_t size, int tag) {
   //log out to a file 
    if(tag != -1){
      enqueueReq(id_counter++, address, size, tag, 0, 0);
    }
    // std::cout << "read_process_memory: address = " << (void*)address << ", size = " << size << ", tag = " << tag << std::endl;
    auto dma_op = std::make_unique<PACDmaReadOp<ZC_DMA_BLOCK_SIZE>>(address, size, tag);
    auto return_dma = protoacc_bm_->ZeroCostBlockingDma(std::move(dma_op));
    memcpy(buffer, return_dma->data, size);
    // std::cout << "read_process_memory: done " << std::endl;
    return 0;
}

ssize_t write_process_memory(uintptr_t address, void *buffer, size_t size, int tag){
    enqueueReq(id_counter++, address, size, tag, 1, buffer);
    auto dma_op = std::make_unique<PACDmaWriteOp>(address, size, tag);
    std::memcpy(dma_op->buffer, buffer, size);
    protoacc_bm_->ZeroCostBlockingDma(std::move(dma_op));
    return 0;
}

typedef enum {
    S_WAIT_CMD = 0,
    S_SCALAR_DISPATCH_REQ = 1,
    S_SCALAR_OUTPUT_DATA = 2,
    S_WRITE_KEY = 3,
    S_STRING_GETPTR = 4,
    S_STRING_GETHEADER1 = 5,
    S_STRING_GETHEADER2 = 6,
    S_STRING_RECVHEADER1 = 7,
    S_STRING_RECVHEADER2 = 8,
    S_STRING_LOADDATA = 9,
    S_STRING_WRITEKEY = 10,
    S_UNPACKED_REP_GETPTR = 11,
    S_UNPACKED_REP_GETSIZE = 12,
    S_UNPACKED_REP_RECVPTR = 13,
    S_UNPACKED_REP_RECVSIZE = 14
} SCALAR_STATES;

typedef enum {
    WIRE_TYPE_VARINT = 0,
    WIRE_TYPE_64bit = 1,
    WIRE_TYPE_LEN_DELIM = 2,
    WIRE_TYPE_START_GROUP = 3,
    WIRE_TYPE_END_GROUP = 4,
    WIRE_TYPE_32bit = 5
} WIRE_TYPES;

WIRE_TYPES wire_type_lookup[19] = {WIRE_TYPE_VARINT, WIRE_TYPE_64bit, WIRE_TYPE_32bit, WIRE_TYPE_VARINT, WIRE_TYPE_VARINT, WIRE_TYPE_VARINT, WIRE_TYPE_64bit, WIRE_TYPE_32bit, WIRE_TYPE_VARINT, WIRE_TYPE_LEN_DELIM, WIRE_TYPE_START_GROUP, WIRE_TYPE_LEN_DELIM, WIRE_TYPE_LEN_DELIM, WIRE_TYPE_VARINT, WIRE_TYPE_VARINT, WIRE_TYPE_32bit, WIRE_TYPE_64bit, WIRE_TYPE_VARINT, WIRE_TYPE_VARINT};

typedef enum {
    TYPE_DOUBLE = 1,
    TYPE_FLOAT = 2,
    TYPE_INT64 = 3,
    TYPE_UINT64 = 4,
    TYPE_INT32 = 5,
    TYPE_FIXED64 = 6,
    TYPE_FIXED32 = 7,
    TYPE_BOOL = 8,
    TYPE_STRING = 9,
    TYPE_GROUP = 10,
    TYPE_MESSAGE = 11,
    TYPE_BYTES = 12,
    TYPE_UINT32 = 13,
    TYPE_ENUM = 14,
    TYPE_SFIXED32 = 15,
    TYPE_SFIXED64 = 16,
    TYPE_SINT32 = 17,
    TYPE_SINT64 = 18,
    TYPE_fieldwidth = 5
}PROTO_TYPES;

int cpp_size[19] = {0, 3, 2, 3, 3, 2, 3, 2, 0, 3, 0, 3, 3, 2, 2, 2, 3, 2, 3};
int is_poentially_scalar[19] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1};
int is_variant_signed[19] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1};


std::stack<uint64_t> size_stack;
std::stack<uint64_t> depth_stack;

static uint64_t backend_string_ptr_output_addr = 0;
static uint64_t backend_stringobj_output_addr_tail = 0;
static uint64_t frontend_stringobj_output_addr_tail = 0;

void protoacc_mem_writer(struct WriteBuddle& outputQ){
    uint64_t len_to_write = outputQ.validbytes;
    // uint64_t obj_addr = frontend_stringobj_output_addr_tail;
    // // this is the ptr to the data
    frontend_stringobj_output_addr_tail = frontend_stringobj_output_addr_tail - len_to_write;
    // each write is at most 16B
    uint64_t bytes_to_write = len_to_write > 16 ? 16 : len_to_write;
    uint64_t write_addr = backend_stringobj_output_addr_tail - bytes_to_write;
    // DEBUG_PRINT("write out addr %p outputQ.validbytes %ld\n", (void*)write_addr, outputQ.validbytes);
    assert(len_to_write <= 16);
    // this can be wrong
    write_process_memory(write_addr, &outputQ.data, bytes_to_write, (int)protoacc::CstStr::WRITE_OUT);
    backend_stringobj_output_addr_tail = backend_stringobj_output_addr_tail - bytes_to_write;
    
}

static uint64_t nonscalar_count = 0;

void protoacc_serialize_field(uint64_t relative_fieldno, uint64_t min_fieldno, uint64_t cpp_obj_addr, uint64_t entry_1, uint64_t entry_2){
  int descr_result_is_repeated = (entry_1 >> 63) & 1;
  int descr_result_typeinfo = (entry_1 << 1 >> 59) & 0x1F;
  // int descr_result_is_nested = descr_result_typeinfo == TYPE_MESSAGE;
  uint64_t descr_result_offset = ((entry_1 << 6) >> 6);

  uint64_t src_data_addr = cpp_obj_addr + descr_result_offset;
  int src_data_type = descr_result_typeinfo;
  uint64_t field_number = relative_fieldno + min_fieldno - 1;
  // depth is missing
  uint64_t end_of_message = relative_fieldno == 0;
  if(end_of_message){
    // !!! this is wrong
    field_number = 0;
  }

  // if end of message needs field_number will equal to parent_fieldnum

  int cpp_size_log2_reg = cpp_size[src_data_type];
  int cpp_size_nonlog2_fromreg = 1 << cpp_size_log2_reg;
  int cpp_size_nonlog2_numbits_fromreg = cpp_size_nonlog2_fromreg << 3;
  int wire_type_reg = wire_type_lookup[src_data_type];
  int is_varint_signed = is_variant_signed[src_data_type];
  int is_int32_reg = src_data_type == TYPE_INT32;
  int detailedTypeIsPotentiallyScalar = is_poentially_scalar[src_data_type];
  int is_bytes_or_string = (src_data_type == TYPE_STRING) || (src_data_type == TYPE_BYTES);
  int is_repeated = descr_result_is_repeated;
  int is_packed = 0;
  int varintDataUnsigned = !is_varint_signed;
  int varintData64bit = cpp_size_log2_reg == 3;

  uint64_t unencoded_key = (field_number << 3) | wire_type_lookup[src_data_type];
  VarintEncode key_encoder;
  key_encoder.inputData = unencoded_key;
  CombinationalVarintEncode(&key_encoder);

  int handlerState = 0;
  // S_WAIT_CMD:
  uint128_t memread = 0;

  uint64_t string_obj_ptr_reg = 0;
  uint64_t string_data_ptr_reg = 0;
  uint64_t base_addr_bytes = 0;
  uint64_t base_len = 0;
  uint64_t aligned_loadlen = 0;
  uint64_t base_addr_start_index = 0;
  uint64_t base_addr_end_index = 0;
  uint64_t base_addr_end_index_inclusive = 0;
  uint64_t extra_word = 0;
  uint64_t base_addr_bytes_aligned = 0;
  uint64_t words_to_load = 0;
  uint64_t words_to_load_minus_one = 0;

  uint128_t encoded_string_length_no_null_term_reg = 0;
  uint64_t encoded_string_length_no_null_term_bytes_reg = 0;
  uint64_t base_addr_bytes_aligned_reg = 0;
  uint64_t words_to_load_reg = 0;
  uint64_t words_to_load_minus_one_reg_fixed = 0;
  uint64_t base_addr_start_index_reg = 0;
  uint64_t base_addr_end_index_inclusive_reg = 0;
  uint64_t string_load_respcounter = 0;

  uint64_t repeated_elems_headptr;

  struct WriteBuddle outputQ;
  outputQ.data = 0;
  outputQ.validbytes = 0;
  struct StoreMemValue store;
  VarintEncode dataencoder;
  VarintEncode lenencoder;

  int bytes_data[1000] = {0};
  int bytes_data_size = 0;
  std::string field_type = "";
  switch (src_data_type){
    case TYPE_STRING:
      field_type = "nonscalar";
      nonscalar_count++;
    break;
    case TYPE_BYTES:
      field_type = "nonscalar"; 
      nonscalar_count++;
    break;
    default:
      field_type = "scalar";
    break;
  }
  int field_is_repeated = is_repeated;

  if(end_of_message){
    DEBUG_PRINT("end of message\n");
    // this some issue here, that need to fix
    VarintEncode msg_size_encoder;
    msg_size_encoder.inputData = size_stack.top();
    CombinationalVarintEncode(&msg_size_encoder);
    
    // https://github.com/ucb-bar/protoacc/blob/d2d69ab9b67ceae64ff5f98120ac99370f30a473/src/main/scala/memwriter_serializer.scala#L77
    outputQ.data = msg_size_encoder.outputData;
    outputQ.validbytes = msg_size_encoder.outputBytes;
    protoacc_mem_writer(outputQ);
    outputQ.validbytes = 0;
    outputQ.data = 0;

    size_stack.top() += outputQ.validbytes;
    // then write out the key 
    // https://github.com/ucb-bar/protoacc/blob/d2d69ab9b67ceae64ff5f98120ac99370f30a473/src/main/scala/memwriter_serializer.scala#L73
    // size_stack(depth) + writes_input_IF_Q.io.deq.bits.validbytes is merged together
    // finally the parent needs to add the size_stack(depth) to the size_stack(depth-1)
    handlerState = S_WRITE_KEY;

  }else{
    if(detailedTypeIsPotentiallyScalar && !is_repeated){
      handlerState = S_SCALAR_DISPATCH_REQ;
    }else if(is_bytes_or_string && !is_repeated){
      handlerState = S_STRING_GETPTR;
    }else if((detailedTypeIsPotentiallyScalar || is_bytes_or_string) && is_repeated){
      handlerState = S_UNPACKED_REP_GETPTR;
    }else{
      assert(0);
    }
  }

  while(1){
    switch(handlerState){
      case S_SCALAR_DISPATCH_REQ:{
        DEBUG_PRINT("S_SCALAR_DISPATCH_REQ, byte data size %d\n", bytes_data_size);
        bytes_data[bytes_data_size++] = 1<<cpp_size_log2_reg;
        DEBUG_PRINT("S_SCALAR_DISPATCH_REQ, size %d \n", 1<<cpp_size_log2_reg);
        assert(cpp_size_log2_reg <= 16);
        read_process_memory(src_data_addr, &memread, 1<<cpp_size_log2_reg, (int)protoacc::CstStr::SCALAR_DISPATCH_REQ);
        handlerState = S_SCALAR_OUTPUT_DATA;
      }
      break;
      case S_SCALAR_OUTPUT_DATA:{
        DEBUG_PRINT("S_SCALAR_OUTPUT_DATA\n");
        uint64_t inputData = compute_values(&store, memread, cpp_size_nonlog2_numbits_fromreg, is_int32_reg, varintDataUnsigned, varintData64bit);
        if(wire_type_reg == WIRE_TYPE_VARINT){
          dataencoder.inputData = inputData;
          CombinationalVarintEncode(&dataencoder);
          outputQ.data = dataencoder.outputData;
          outputQ.validbytes = dataencoder.outputBytes;
        }else{
          outputQ.data = store.mem_resp_raw;
          outputQ.validbytes = cpp_size_nonlog2_fromreg;
        }
        if(!(is_repeated && is_packed)){
          handlerState = S_WRITE_KEY;
        }else{
          DEBUG_PRINT("DEBUG=== src_data_addr and repeated_elems_headptr %p %p\n", (void*)src_data_addr, (void*)repeated_elems_headptr);
          if(src_data_addr == repeated_elems_headptr){
            handlerState = S_WRITE_KEY;
          }else{
            src_data_addr = src_data_addr - cpp_size_nonlog2_fromreg;
            handlerState = S_SCALAR_DISPATCH_REQ;
          }
        }
      }
      break;
      case S_WRITE_KEY:{

        DEBUG_PRINT("S_WRITE_KEY\n");
        outputQ.data = key_encoder.outputData;
        outputQ.validbytes = key_encoder.outputBytes;
        int is_unpacked_repeated = is_repeated && !is_packed;
        if(!is_unpacked_repeated){
           // for json
          if(end_of_message){

          }else{
            add_field(std::to_string(field_number), field_type, field_is_repeated, bytes_data, bytes_data_size);
            bytes_data_size = 0;
          }

          handlerState = S_WAIT_CMD;
        }else {
          if(src_data_addr == repeated_elems_headptr){
            
            // for json
            if(end_of_message){

            }else{
              add_field(std::to_string(field_number), field_type, field_is_repeated, bytes_data, bytes_data_size);
              bytes_data_size = 0;
            }

            handlerState = S_WAIT_CMD;
          }else{
            src_data_addr = src_data_addr - cpp_size_nonlog2_fromreg;
            handlerState = S_SCALAR_DISPATCH_REQ;
          }
        }
      }
      break;
    
      case S_STRING_GETPTR: {
        DEBUG_PRINT("S_STRING_GETPTR src_data_addr %p, size %d \n", (void*)src_data_addr, 1<<cpp_size_log2_reg);
        read_process_memory(src_data_addr, &memread, 1<<cpp_size_log2_reg, (int)protoacc::CstStr::STRING_GETPTR_REQ);
        handlerState = S_STRING_GETHEADER1;
      }
      break;
      case S_STRING_GETHEADER1:{
        DEBUG_PRINT("S_STRING_GETHEADER1\n");
        string_obj_ptr_reg = (uint64_t)(memread & 0xFFFFFFFFFFFFFFFF);
        read_process_memory(string_obj_ptr_reg, &string_data_ptr_reg, 8, (int)protoacc::CstStr::STRING_GETPTR_REQ);

        handlerState = S_STRING_GETHEADER2;
      }
      break;
      case S_STRING_GETHEADER2:{
        DEBUG_PRINT("S_STRING_GETHEADER2\n");
        // https://github.com/ucb-bar/protoacc/blob/d2d69ab9b67ceae64ff5f98120ac99370f30a473/src/main/scala/fieldhandler_serializer.scala#L347
        read_process_memory(string_obj_ptr_reg+8, &base_len, 8, (int)protoacc::CstStr::STRING_GETPTR_REQ);
        handlerState = S_STRING_RECVHEADER1;
      }
      break;
      case S_STRING_RECVHEADER1:{
        DEBUG_PRINT("S_STRING_RECVHEADER1\n");
        handlerState = S_STRING_RECVHEADER2;
      }
      break;
      case S_STRING_RECVHEADER2:{

        // for json
        bytes_data[bytes_data_size++] = base_len;

        DEBUG_PRINT("S_STRING_RECVHEADER2, base_len %ld\n", base_len);
        base_addr_bytes = string_data_ptr_reg;
        //base_len = base_len;
        base_addr_start_index = base_addr_bytes & 0xF;
        aligned_loadlen = base_len + base_addr_start_index;
        base_addr_end_index = aligned_loadlen & 0xF;
        base_addr_end_index_inclusive = (aligned_loadlen - 1) & 0xF;
        extra_word = (aligned_loadlen & 0xF) != 0;
        base_addr_bytes_aligned = (base_addr_bytes >> 4) << 4;
        words_to_load = (aligned_loadlen >> 4) + extra_word;
        words_to_load_minus_one = words_to_load - 1;
        lenencoder.inputData = base_len;
        CombinationalVarintEncode(&lenencoder);
        
        encoded_string_length_no_null_term_reg = lenencoder.outputData;
        encoded_string_length_no_null_term_bytes_reg = lenencoder.outputBytes;
        base_addr_bytes_aligned_reg = base_addr_bytes_aligned;
        words_to_load_reg = words_to_load;
        words_to_load_minus_one_reg_fixed = words_to_load_minus_one;

        base_addr_start_index_reg = base_addr_start_index;
        base_addr_end_index_inclusive_reg = base_addr_end_index_inclusive;

        handlerState = S_STRING_LOADDATA;
      }
      break;
      case S_STRING_LOADDATA:{
        // DEBUG_PRINT("S_STRING_LOADDATA words_to_load_reg %ld, string_load_respcounter %ld, words_to_load_minus_one %ld\n", words_to_load_reg, string_load_respcounter, words_to_load_minus_one_reg_fixed);
        if(words_to_load_reg != 0){
          uint64_t addr = base_addr_bytes_aligned_reg + ((words_to_load_reg-1) << 4);
          read_process_memory(addr, &memread, 16, (int)protoacc::CstStr::STRING_LOADDATA_REQ);
          // DEBUG_PRINT("memread %lx, %lx\n", (uint64_t)(memread & 0xFFFFFFFFFFFFFFFF), (uint64_t)(memread >> 64));
          words_to_load_reg = words_to_load_reg - 1;
        }

        int handlingtail = string_load_respcounter == 0;
        int handlingfront = string_load_respcounter == words_to_load_minus_one_reg_fixed;

        outputQ.data = handlingfront ? (memread >> (base_addr_start_index_reg << 3)) : memread;
        if(handlingfront && handlingtail){
          outputQ.validbytes = (base_addr_end_index_inclusive_reg - base_addr_start_index_reg) + 1;
        }else if(handlingtail){
          outputQ.validbytes = (base_addr_end_index_inclusive_reg + 1);
        }else if(handlingfront){
          outputQ.validbytes = 16 - base_addr_start_index_reg;
        }else{
          outputQ.validbytes = 16;
        }

        if(handlingfront){
          handlerState = S_STRING_WRITEKEY;
          string_load_respcounter = 0;
        }else{
          string_load_respcounter = string_load_respcounter + 1;
        }
      }
      break;
      case S_STRING_WRITEKEY:{
        

        DEBUG_PRINT("S_STRING_WRITEKEY\n");
        outputQ.data =  key_encoder.outputData | (encoded_string_length_no_null_term_reg << (key_encoder.outputBytes << 3));
        outputQ.validbytes = key_encoder.outputBytes + encoded_string_length_no_null_term_bytes_reg;
        int is_unpacked_repeated = is_repeated && !is_packed;
        if(!is_unpacked_repeated){
            // for json
            add_field(std::to_string(field_number), field_type, field_is_repeated, bytes_data, bytes_data_size);
            bytes_data_size = 0;
            
            handlerState = S_WAIT_CMD;
        }else{
          if(src_data_addr == repeated_elems_headptr){
            // for json
            add_field(std::to_string(field_number), field_type, field_is_repeated, bytes_data, bytes_data_size);
            bytes_data_size = 0;
            
            handlerState = S_WAIT_CMD;
          }else{
            src_data_addr = src_data_addr - cpp_size_nonlog2_fromreg;
            handlerState = S_STRING_GETPTR;
          }
        }
      }
      break;
      case S_UNPACKED_REP_GETPTR:{
        DEBUG_PRINT("S_UNPACKED_REP_GETPTR\n");
        if(is_bytes_or_string){
          read_process_memory(src_data_addr+8, &repeated_elems_headptr, 8, (int)protoacc::CstStr::UNPACKED_REP_GETPTR_REQ);
        }else{
          read_process_memory(src_data_addr, &repeated_elems_headptr, 8, (int)protoacc::CstStr::UNPACKED_REP_GETPTR_REQ);
        }
        handlerState = S_UNPACKED_REP_GETSIZE;
      }
      break;
      case S_UNPACKED_REP_GETSIZE:{
        DEBUG_PRINT("S_UNPACKED_REP_GETSIZE\n");
        if(is_bytes_or_string){
          read_process_memory(src_data_addr, &memread, 8, (int)protoacc::CstStr::UNPACKED_REP_GETPTR_REQ);
        }else{
          read_process_memory(src_data_addr-8, &memread, 8, (int)protoacc::CstStr::UNPACKED_REP_GETPTR_REQ);
        }
        handlerState = S_UNPACKED_REP_RECVPTR;
      }
      break;
      case S_UNPACKED_REP_RECVPTR:{
        DEBUG_PRINT("S_UNPACKED_REP_RECVPTR\n");
        if(is_bytes_or_string){
          repeated_elems_headptr += 8;
        }
        handlerState = S_UNPACKED_REP_RECVSIZE;
      }
      break;
      case S_UNPACKED_REP_RECVSIZE:{
        DEBUG_PRINT("S_UNPACKED_REP_RECVSIZE\n");
        int num_elems = (int)(memread & 0xFFFFFFFF);
        uint64_t ptr_to_last_elem = repeated_elems_headptr + ((num_elems-1) << cpp_size_log2_reg); 
        src_data_addr = ptr_to_last_elem;
        if(is_bytes_or_string){
          handlerState = S_STRING_GETPTR;
        }else{
          handlerState = S_SCALAR_DISPATCH_REQ;
        }
      }
      break;
      default:
        assert(0);
    }

    if(outputQ.validbytes != 0){
      size_stack.top() += outputQ.validbytes;
      protoacc_mem_writer(outputQ);
      // at end of toplevel, the top-level message address also needs to be written out backend_string_ptr_output_addr , omitted here
      // https://github.com/ucb-bar/protoacc/blob/d2d69ab9b67ceae64ff5f98120ac99370f30a473/src/main/scala/memwriter_serializer.scala#L160
      // https://github.com/ucb-bar/protoacc/blob/d2d69ab9b67ceae64ff5f98120ac99370f30a473/src/main/scala/memwriter_serializer.scala#L278
      // backend_string_ptr_output_addr + 8 later

      //reset
      outputQ.validbytes = 0;
      outputQ.data = 0;
    }

    if(handlerState == S_WAIT_CMD){
      DEBUG_PRINT("End of field handle\n");
      break;
    }
  }
}

static uint64_t depth = 0;
void parse_message_to_json(uint64_t descriptor_table_addr, uint64_t cpp_obj_addr) {
    depth_stack.push(depth++);
    size_stack.push(0);
    DEBUG_PRINT("=== messsage start ===\n");
    // Read the message
    void* buffer = malloc(4096);
    memset(buffer, 0, 4096);
    read_process_memory(descriptor_table_addr, buffer, sizeof(uint64_t)*3, -1);
   // header has 64B
    uint8_t* header = (uint8_t*) malloc(64);
    read_process_memory(descriptor_table_addr, header, 64, -1);

    
    //first 8 bytes 
    void* vptr = (void*)((uint64_t*)header)[0];
    
    //second 8bytes
    uint64_t size = ((uint64_t*)header)[1];
    
    //third 8bytes
    uint64_t hasbits_off = ((uint64_t*)header)[2];
    
    // fouth 8bytes
    uint64_t min_max_field = ((uint64_t*)header)[3];

    DEBUG_PRINT("cpp_obj_addr %lx\n", cpp_obj_addr);
    // DEBUG_PRINT("Descriptor table: vptr %lx size %lx hasbits_off%lx\n", ((uint64_t*)buffer)[0], ((uint64_t*)buffer)[1], ((uint64_t*)buffer)[2]);
    DEBUG_PRINT("hasbits : %lx\n", hasbits_off);
    DEBUG_PRINT("min_max_fieldno : %lx\n", min_max_field);

    uint64_t max_fieldno = min_max_field & 0x00000000FFFFFFFFL;
    uint64_t min_fieldno = min_max_field >> 32; 
    DEBUG_PRINT("max_field %ld, min_field %ld\n", max_fieldno, min_fieldno);
    uint64_t depth_plus_one = max_fieldno - min_fieldno + 1;
    uint64_t hasbits_addr = (uint64_t)(cpp_obj_addr + hasbits_off);

    // every field is 128bits = 16B = 2^4
    uint64_t is_submessage_base = ((max_fieldno - min_fieldno + 1) << 4)+32+descriptor_table_addr;

    // or hasbits_max_bitoffset 
    uint64_t current_has_bits_next_bitoffset = max_fieldno - min_fieldno + 1;
    uint64_t has_bits_max_bitoffset = current_has_bits_next_bitoffset;
    // val next_next_field_offset = (current_has_bits_next_bitoffset % 32.U) + 1.U
    // current_has_bits_next_bitoffset := current_has_bits_next_bitoffset - next_next_field_offset

    // divide by 32 to get the array start
    // load 4 bytes for 32 bits of has bits
    uint64_t hasbits_array_index; 
    uint64_t hasbits_request_addr;
    uint64_t is_submessage_request_addr;
    uint64_t hasbits_resp_fieldno;

    // hasbit first bit is not used 
    uint64_t num_fields_this_hasbits, fieldno_offset_from_tail=0;

    #define ACCESS_ONE_BIT(data, bit_pos) ((data >> (bit_pos)) & 1)

    #define ACCESS_FIELD_START(filedno)  (((filedno-1) << 4)+32+descriptor_table_addr)

    while(1){
        // load max 32 fields
        uint32_t hasbits_4bytes=0;
        uint32_t is_submessage_4bytes=0;
        num_fields_this_hasbits = current_has_bits_next_bitoffset % 32 + 1;
        hasbits_array_index = current_has_bits_next_bitoffset >> 5;
        hasbits_request_addr = (hasbits_array_index << 2) + hasbits_addr;
        is_submessage_request_addr = (hasbits_array_index << 2) + is_submessage_base;
        read_process_memory(hasbits_request_addr, &hasbits_4bytes, 4, (int)protoacc::CstStr::LOAD_HASBITS_AND_IS_SUBMESSAGE);
        read_process_memory(is_submessage_request_addr, &is_submessage_4bytes, 4, (int)protoacc::CstStr::LOAD_HASBITS_AND_IS_SUBMESSAGE);
        DEBUG_PRINT("hasbits addr %p, hasbits_4bytes %x, is_submessage_4bytes %x\n", (void*)hasbits_request_addr, hasbits_4bytes, is_submessage_4bytes);
        int hasbits_done_chunk = 0;
        while(!hasbits_done_chunk && num_fields_this_hasbits > 0){
          hasbits_resp_fieldno = has_bits_max_bitoffset-fieldno_offset_from_tail;
          int hasbit_for_current_fieldno = ACCESS_ONE_BIT(hasbits_4bytes, hasbits_resp_fieldno%32) || hasbits_resp_fieldno == 0;
          int is_submessage_bit_for_current_fieldno = ACCESS_ONE_BIT(is_submessage_4bytes, hasbits_resp_fieldno%32);
          int current_field_is_present_and_submessage =  hasbit_for_current_fieldno && is_submessage_bit_for_current_fieldno;
          // process the current field
          if(hasbit_for_current_fieldno == 1 && is_submessage_bit_for_current_fieldno == 0){
            // DEBUG_PRINT("I get a field %ld, is present %d, is submessage %d \n", hasbits_resp_fieldno+min_fieldno-1, hasbit_for_current_fieldno, is_submessage_bit_for_current_fieldno);
            uint64_t field_addr = ACCESS_FIELD_START(hasbits_resp_fieldno);
            uint64_t field_entry[2];
            read_process_memory(field_addr, &field_entry, 16, (int)protoacc::CstStr::LOAD_EACH_FIELD);
            protoacc_serialize_field(hasbits_resp_fieldno, min_fieldno, cpp_obj_addr, field_entry[0], field_entry[1]);
            // uint64_t cpp_obj_field_addr = cpp_obj_addr + ((field_entry[0] << 6) >> 6);
            // serialize_field(mem_fd, cpp_obj_addr, hasbits_resp_fieldno+min_fieldno-1);
          }
          fieldno_offset_from_tail++;
          int hasbits_chunk_end = fieldno_offset_from_tail == num_fields_this_hasbits;
          // hasbits_done_chunk = hasbits_chunk_end || current_field_is_present_and_submessage;
          if(current_field_is_present_and_submessage){
            uint64_t field_addr = ACCESS_FIELD_START(hasbits_resp_fieldno);
            uint64_t buffer[2];
            read_process_memory(field_addr, &buffer, 16, (int)protoacc::CstStr::LOAD_NEW_SUBMESSAGE);
            uint64_t descriptor_table_addr = buffer[1];
            // https://github.com/ucb-bar/protoacc/blob/master/src/main/scala/descriptortablehandler_serializer.scala#L471
            uint64_t new_cpp_obj_addr=0;
            read_process_memory(cpp_obj_addr + ((buffer[0] << 6) >> 6), &new_cpp_obj_addr, 8, (int)protoacc::CstStr::LOAD_NEW_SUBMESSAGE);
            
            new_submessage();
            parse_message_to_json(descriptor_table_addr, new_cpp_obj_addr);
            end_submessage(std::to_string(hasbits_resp_fieldno+min_fieldno-1), false);
            uint64_t child_size = size_stack.top(); 
            size_stack.pop();
            size_stack.top() += child_size;
            depth_stack.pop();
          }
          hasbits_done_chunk = hasbits_chunk_end;
        }
        fieldno_offset_from_tail = 0;
        if(current_has_bits_next_bitoffset <= 31){
          // done already
          break;
        }
        current_has_bits_next_bitoffset = current_has_bits_next_bitoffset - (current_has_bits_next_bitoffset%32+1);
        has_bits_max_bitoffset = current_has_bits_next_bitoffset;
        DEBUG_PRINT("current_has_bits_next_bitoffset %ld\n", current_has_bits_next_bitoffset);
    }
    
    DEBUG_PRINT("=== messsage ends ===\n");
    return ;
    //is_submessage_base_addr := (((max_fieldno - min_fieldno) + 1.U) << 4) + 32.U + io.serializer_cmd_in.bits.descriptor_table_addr
    // 2^4 is 16 bytes, 32 bytes for the header, 
    // each entry now is 128bits = 16B
}

extern "C"{
    void protoacc_func_setup_output(uint64_t, uint64_t);
    void protoacc_func_sim(uint64_t, uint64_t);
}

void protoacc_func_setup_output(uint64_t string_ptr_output_addr, uint64_t stringobj_output_addr){
  backend_string_ptr_output_addr = string_ptr_output_addr;
  backend_stringobj_output_addr_tail = stringobj_output_addr;
  frontend_stringobj_output_addr_tail = stringobj_output_addr;
}

void protoacc_func_sim(uint64_t descriptor_table_addr, uint64_t src_base_addr){

// Each ADT contains three regions. The 64B header region con-
// tains layout information at the message-level, consisting of: (1) a
// pointer to a default instance (or vptr value) of the message type,
// (2) the size of C++ objects of the message type, (3) an offset into
// message objects for an array of field-presence bit fields (hasbits),
// and (4) the min and max field number defined in the message. The
// second ADT region consists of 128-bit wide entries that represent
// each field in the message type, indexed by field number. Each entry
// consists of the following details for a field: (1) the field’s C++ type
// and whether the field is repeated, (2) the offset where the field
// begins in the in-memory C++ representation of the message, and
// (3) for sub-message fields, a pointer to the sub-message type’s ADT.
// The final ADT region is the is_submessage bit field, an array of
// bits that indicates if a field is a sub-message. This is used to reduce
// complexity in the serializer, since it can know when it needs to
// switch contexts into a sub-message without waiting for a full ADT
// entry read.
  
//example:
// alignas(16) const ::PROTOBUF_NAMESPACE_ID::uint64  primitivetests_FriendStruct_Paccser_PaccboolMessageMessage_ACCEL_DESCRIPTORS::Paccser_PaccboolMessageMessage_ACCEL_DESCRIPTORS[] = {
//   /* HEADER: */
//   /* entry 0: this obj vptr */
//    (uint64_t)(*((uint64_t*)(&(::primitivetests::Paccser_PaccboolMessageMessage::default_instance())))),
//   /* entry 1: this obj size */
//    (uint64_t)sizeof(::primitivetests::Paccser_PaccboolMessageMessage),
//   /* entry 2: hasbits raw offset */
//    (uint64_t) PROTOBUF_FIELD_OFFSET(::primitivetests::Paccser_PaccboolMessageMessage, _has_bits_),
//   /* entry 3: */
//   /* min field num */ (((uint64_t) 1L) << 32) |
//   /* max field num */ (((uint64_t) 1L) & 0x00000000FFFFFFFFL),

//   /* ENTRIES (128 bits each): */
//   /* { is_repeated (1bit) | cpp_type (5bits) | offset (58bits) } */
//   /* { submessage ADT pointer (64 bits) } */

//   /* field 1 entry */
//   /* is_repeated */
//   (((uint64_t)0) << 63) |
//   /*        type */
//   ((((uint64_t)11) & 0x1F) << 58) |
//   /*      offset */
//   (((((uint64_t)( PROTOBUF_FIELD_OFFSET(::primitivetests::Paccser_PaccboolMessageMessage, paccpaccser_boolmessage_0_) ))) << 6) >> 6),
//   /* if nested message, pointer to that type's descriptor table */
//   (uint64_t)(primitivetests_FriendStruct_Paccser_boolMessage_ACCEL_DESCRIPTORS::Paccser_boolMessage_ACCEL_DESCRIPTORS),

//   /* is_submessage region (64 bits each): */
//   2L,
// };
  printf(" ===== protoacc func sim descriptor_table_addr %lx, src_base_addr %lx\n", descriptor_table_addr, src_base_addr);
  new_message();
  parse_message_to_json(descriptor_table_addr, src_base_addr);
  end_message();

  printf(" ===== nonscalar_count %ld\n", nonscalar_count);
   
  return;
  
}

// when write out submessage, 
// it first issue outputQ with 0 bytes, but sub_message == True, last_for_arbitration_round == 0.
// it then issue key (parent field number) of the submessage, with x bytes, and sub_message == True, last_for_arbitration_round == 1.
// when memwriter sees the first write, it execute 
//     val enc_len = varint_encoder.io.outputData
//         val enc_len_bytes_write = varint_encoder.io.outputBytes
//         write_inject_Q.io.enq.bits.data := enc_len
//         write_inject_Q.io.enq.bits.validbytes := enc_len_bytes_write
//         when (write_inject_Q.io.enq.fire()) {
//           size_stack(depth) := size_stack(depth) + enc_len_bytes_write
//     }
// when memwriter sees the next write, it execute 
//     depth := depth_minus_one
//     size_stack(depth_minus_one) := size_stack(depth_minus_one) + size_stack(depth) + writes_input_IF_Q.io.deq.bits.validbytes
//     size_stack(depth) := 0.U