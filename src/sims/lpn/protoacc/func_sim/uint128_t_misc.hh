#include <stdint.h>
#include <bits/stdint-uintn.h>
#include <stdio.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <sys/types.h>
using boost::multiprecision::uint128_t;

struct StoreMemValue {
    uint128_t mem_resp_raw;
    uint128_t mem_resp_zigzag32;
    uint128_t mem_resp_zigzag64;
};

uint64_t compute_values(struct StoreMemValue* store, uint128_t mem_data, uint32_t cpp_size_nonlog2_numbits_fromreg, int is_int32_reg, int varintDataUnsigned, int varintData64bit) {

    uint128_t mem_resp_raw = 0;
    uint128_t mem_resp_zigzag32 = 0;
    uint128_t mem_resp_zigzag64 = 0;

    // read_mask := ((1.U << (cpp_size_nonlog2_numbits_fromreg)) - 1.U)
    uint128_t read_mask = ((uint128_t)1 << cpp_size_nonlog2_numbits_fromreg) - 1;

    // Compute ORMASK: ((1 << 32) - 1) shifted left by 32 bits
    uint128_t ORMASK = (((uint128_t)1 << 32) - 1) << 32;
    // ORMASK now has ones in bits [63:32], zeros elsewhere.

    // Compute mem_resp_masked
    uint128_t mem_resp_masked = mem_data & read_mask;

    // Determine if bit 31 of mem_resp_masked is set
    bool mem_resp_masked_bit31 = ((mem_resp_masked >> 31) & 1) != 0;

    // Compute maybe_extended_int32val using a conditional operator (equivalent to Mux)
    uint128_t maybe_extended_int32val = mem_resp_masked_bit31
        ? (ORMASK | mem_resp_masked)
        : mem_resp_masked;

    // Compute mem_resp_raw based on is_int32_reg
    mem_resp_raw = is_int32_reg
        ? maybe_extended_int32val
        : mem_resp_masked;

    // Compute mem_resp_zigzag32
    // Check if bit 31 of mem_resp_raw is set
    bool mem_resp_raw_bit31 = ((mem_resp_raw >> 31) & 1) != 0;
    // Create a 32-bit mask of all ones or zeros based on mem_resp_raw_bit31
    uint128_t mask32 = mem_resp_raw_bit31 ? (uint128_t)0xFFFFFFFF : (uint128_t)0;
    // Perform the zigzag operation
    mem_resp_zigzag32 = (mem_resp_raw << 1) ^ mask32;

    // Compute mem_resp_zigzag64
    // Check if bit 63 of mem_resp_raw is set
    bool mem_resp_raw_bit63 = ((mem_resp_raw >> 63) & 1) != 0;
    // Create a 64-bit mask of all ones or zeros based on mem_resp_raw_bit63
    uint128_t mask64 = mem_resp_raw_bit63 ? ~((uint128_t)0) : (uint128_t)0;
    // Perform the zigzag operation
    mem_resp_zigzag64 = (mem_resp_raw << 1) ^ mask64;

    // printf("mem_resp_zigzag32: %llu\n", (unsigned long long)(mem_resp_zigzag32 & 0xFFFFFFFFFFFFFFFFULL));
    // printf("mem_resp_zigzag64: %llu\n", (unsigned long long)(mem_resp_zigzag64 & 0xFFFFFFFFFFFFFFFFULL));

    store->mem_resp_raw = mem_resp_raw;
    store->mem_resp_zigzag32 = mem_resp_zigzag32;
    store->mem_resp_zigzag64 = mem_resp_zigzag64;
    
    uint128_t result;
    if(varintDataUnsigned) {
        result = mem_resp_raw;
    } else if(varintData64bit) {
        result = mem_resp_zigzag64;
    } else {
        result = mem_resp_zigzag32;
    }
    return (uint64_t)(result & 0xFFFFFFFFFFFFFFFFULL);
}

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t inputData;
    uint128_t outputData;  // Bits [79:0], up to 80 bits
    int outputBytes;
} VarintEncode;

void CombinationalVarintEncode(VarintEncode *io) {
    // Extracting chunks of bits
    uint8_t chunk[10];
    chunk[0] = io->inputData & 0x7F;            // Bits [6:0]
    chunk[1] = (io->inputData >> 7) & 0x7F;     // Bits [13:7]
    chunk[2] = (io->inputData >> 14) & 0x7F;    // Bits [20:14]
    chunk[3] = (io->inputData >> 21) & 0x7F;    // Bits [27:21]
    chunk[4] = (io->inputData >> 28) & 0x7F;    // Bits [34:28]
    chunk[5] = (io->inputData >> 35) & 0x7F;    // Bits [41:35]
    chunk[6] = (io->inputData >> 42) & 0x7F;    // Bits [48:42]
    chunk[7] = (io->inputData >> 49) & 0x7F;    // Bits [55:49]
    chunk[8] = (io->inputData >> 56) & 0x7F;    // Bits [62:56]
    chunk[9] = (io->inputData >> 63) & 0x01;    // Bit [63]

    // Determining whether chunks need the inclusion bit
    bool chunk_includebit[10];
    chunk_includebit[9] = false;  // Highest chunk has no inclusion bit
    bool higher_chunks_nonzero = (chunk[9] != 0);

    for (int i = 8; i >= 0; i--) {
        chunk_includebit[i] = higher_chunks_nonzero;
        if (chunk[i + 1] != 0) {
            higher_chunks_nonzero = true;
        }
    }

    // Concatenate the chunks into the outputData (bitwise concatenation)
    uint128_t outputData = 0;

    for (int i = 0; i <= 9; i++) {
        uint8_t output_byte = (chunk_includebit[i] << 7) | chunk[i];
        outputData |= ((uint128_t)output_byte) << (i * 8);
    }

    io->outputData = outputData;

    // Counting the number of included chunks (outputBytes)
    int outputBytes = 1;  // Start with 1 for the first chunk
    for (int i = 0; i <= 8; i++) {
        outputBytes += chunk_includebit[i];
    }
    io->outputBytes = outputBytes;
}

struct WriteBuddle{
    uint128_t data;
    uint64_t validbytes;
};