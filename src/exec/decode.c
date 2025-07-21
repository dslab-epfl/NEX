#include <exec/exec.h>
#include <exec/decode.h>
#include <exec/ptrace.h>
#include <sys/uio.h>
reg_map_entry_t capstone_to_mcontext[X86_REG_ENDING] = {

#define REG_ZERO -1
#define REG_NONE -2

    // base (undefined)
    // name, little endian, size
    [0 ... X86_REG_ENDING - 1] = {REG_NONE, 0, 0},
    [X86_REG_AH] = {REG_RAX, 1, 1},
    [X86_REG_AL] = {REG_RAX, 0, 1},
    [X86_REG_AX] = {REG_RAX, 0, 2},
    [X86_REG_EAX] = {REG_RAX, 0, 4},
    [X86_REG_RAX] = {REG_RAX, 0, 8},

    [X86_REG_BH] = {REG_RBX, 1, 1},
    [X86_REG_BL] = {REG_RBX, 0, 1},
    [X86_REG_BX] = {REG_RBX, 0, 2},
    [X86_REG_EBX] = {REG_RBX, 0, 4},
    [X86_REG_RBX] = {REG_RBX, 0, 8},

    [X86_REG_CH] = {REG_RCX, 1, 1},
    [X86_REG_CL] = {REG_RCX, 0, 1},
    [X86_REG_CX] = {REG_RCX, 0, 2},
    [X86_REG_ECX] = {REG_RCX, 0, 4},
    [X86_REG_RCX] = {REG_RCX, 0, 8},

    [X86_REG_DH] = {REG_RDX, 1, 1},
    [X86_REG_DL] = {REG_RDX, 0, 1},
    [X86_REG_DX] = {REG_RDX, 0, 2},
    [X86_REG_EDX] = {REG_RDX, 0, 4},
    [X86_REG_RDX] = {REG_RDX, 0, 8},

    [X86_REG_SIL] = {REG_RSI, 0, 1},
    [X86_REG_SI] = {REG_RSI, 0, 2},
    [X86_REG_ESI] = {REG_RSI, 0, 4},
    [X86_REG_RSI] = {REG_RSI, 0, 8},

    [X86_REG_DIL] = {REG_RDI, 0, 1},
    [X86_REG_DI] = {REG_RDI, 0, 2},
    [X86_REG_EDI] = {REG_RDI, 0, 4},
    [X86_REG_RDI] = {REG_RDI, 0, 8},

    [X86_REG_SPL] = {REG_RSP, 0, 1},
    [X86_REG_SP] = {REG_RSP, 0, 2},
    [X86_REG_ESP] = {REG_RSP, 0, 4},
    [X86_REG_RSP] = {REG_RSP, 0, 8},

    [X86_REG_BPL] = {REG_RBP, 0, 1},
    [X86_REG_BP] = {REG_RBP, 0, 2},
    [X86_REG_EBP] = {REG_RBP, 0, 4},
    [X86_REG_RBP] = {REG_RBP, 0, 8},

#define SANE_GPR(x)                                                       \
  [X86_REG_##x##B] = {REG_##x, 0, 1}, [X86_REG_##x##W] = {REG_##x, 0, 2}, \
  [X86_REG_##x##D] = {REG_##x, 0, 4}, [X86_REG_##x] = {REG_##x, 0, 8}

    SANE_GPR(R8),
    SANE_GPR(R9),
    SANE_GPR(R10),
    SANE_GPR(R11),
    SANE_GPR(R12),
    SANE_GPR(R13),
    SANE_GPR(R14),
    SANE_GPR(R15),

    [X86_REG_IP] = {REG_RIP, 0, 2},
    [X86_REG_EIP] = {REG_RIP, 0, 4},
    [X86_REG_RIP] = {REG_RIP, 0, 8},

    [X86_REG_FS] = {REG_CSGSFS, 4, 2},
    [X86_REG_GS] = {REG_CSGSFS, 2, 2},

    [X86_REG_EFLAGS] = {REG_EFL, 0, 4},

    // pseudo reg that is zero
    [X86_REG_EIZ] = {REG_ZERO, 0, 4},
    [X86_REG_RIZ] = {REG_ZERO, 0, 8},

};

uint64_t compute_effective_address(cs_x86_op* op, ucontext_t *ctx) {
    // Obtain actual register values from context
    reg_map_entry_t* mem_base_reg = CAPSTONE_TO_MCONTEXT(op->mem.base);
    reg_map_entry_t* mem_index_reg = CAPSTONE_TO_MCONTEXT(op->mem.index); 
    uint64_t base_value = (op->mem.base != X86_REG_INVALID) ? ctx->uc_mcontext.gregs[MCREG(mem_base_reg)] : 0;
    uint64_t index_value = (op->mem.index != X86_REG_INVALID) ? ctx->uc_mcontext.gregs[MCREG(mem_index_reg)] : 0;
    return op->mem.disp + base_value + index_value * op->mem.scale;
}

// TODO add cache to avoid re-decoding the same instruction
// Function to analyze an instruction and fill the result struct
int decode(ucontext_t *ctx, decoded_insn_info* result, pid_t child) {
  csh handle;
  uint64_t rip = ctx->uc_mcontext.gregs[REG_RIP];
  DEBUG_MMIO("RIP: %p\n", (void *)rip);
  cs_insn *insn;
  result->pc = rip;
  if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) == CS_ERR_OK) {
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON); // Enable detail mode
    uint8_t rip_[16];
    DEBUG_MMIO("potential points of deadlock\n");
    // potential points of deadlock
    ptrace_read(child, rip, rip_, 16);
    DEBUG_MMIO("no deadlock\n");
    size_t count = cs_disasm(handle, (uint8_t *)rip_, 15, rip, 0, &insn);
    if (count > 0) {
        DEBUG_MMIO("Faulted instruction: %s %s\n", insn[0].mnemonic, insn[0].op_str);
        result->size = insn[0].size;
        cs_x86 *x86 = &(insn[0].detail->x86);
        // Initialize variables to determine operation type
        bool has_memory_destination = false;
        bool has_memory_source = false;

        for (int i = 0; i < x86->op_count; i++) {
            cs_x86_op *op = &x86->operands[i];
            if (op->type == X86_OP_MEM) {
                
                uint64_t address = compute_effective_address(op, ctx);
                result->mem_addr = address;
                result->mem_size = op->size;
                DEBUG_MMIO("Faulted addr %p \n", (void *)address);
                if (i == 0) {  // The first operand in x86 is typically the destination
                    has_memory_destination = true;
                } else {
                    has_memory_source = true;
                }
            }
            if (op->type == X86_OP_REG) {  // Consider the first register as the destination
                result->reg = op->reg;
                DEBUG_MMIO("Register: %s\n", cs_reg_name(handle, op->reg));
            }
            if(op->type == X86_OP_IMM) {
                result->imm_valid = 1;
                result->imm = op->imm;
                DEBUG_MMIO("Immediate: %lu\n", op->imm);
            }
        }

        // Determine read or write based on operand analysis
        if (has_memory_destination && !has_memory_source)
            result->rw = 'W';  // Writing to memory
        else if (!has_memory_destination && has_memory_source)
            result->rw = 'R';  // Reading from memory
        else if (has_memory_destination && has_memory_source)
            result->rw = 'B';  // Both read and write (e.g., moving data between memory locations)
        cs_free(insn, count);
    } else {
        DEBUG("Failed to disassemble instruction\n");
        assert(0);
    }
    safe_printf("step 4.3.3\n");
    cs_close(&handle);
  } else {
    DEBUG("Failed to initialize disassembler!\n");
    assert(0);
  }
  DEBUG_MMIO("Decoded instruction\n");
  return 0;
}
