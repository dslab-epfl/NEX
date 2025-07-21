#pragma once
#include <stdint.h>
#include <sys/ucontext.h>
#include <capstone/capstone.h>


typedef int reg_map_entry_t[3];
typedef struct {
    uint64_t pc;
    int imm_valid;
    uint64_t imm;
    int reg;        // Register involved in capstone
    uint64_t mem_addr;   // Computed memory address (base + index * scale + displacement)
    int mem_size;   // Size of the memory operation
    char rw;     // 'R' for read, 'W' for write, 'B' for both, 'N' for none
    uint32_t size;          // Size of the instruction
} decoded_insn_info;

extern reg_map_entry_t capstone_to_mcontext[X86_REG_ENDING];

#define CAPSTONE_TO_MCONTEXT(r) (&(capstone_to_mcontext[r]))
#define MCREG(m) ((*m)[0])
#define MCOFF(m) ((*m)[1])
#define MCSIZE(m) ((*m)[2])


int decode(ucontext_t *ctx, decoded_insn_info* result, pid_t child);
