#include "decoder.h"
#include <stdlib.h>
#include <string.h>

#define MASK_BITS(start, length) (((1ULL << (length)) - 1) << (start))
#define EXTRACT_BITS(value, start, length) ((value >> start) & ((1ULL << length) - 1))

#define CLASS_DATA_PROCESSING_IMM   0x11000000
#define CLASS_BRANCHES              0x14000000
#define CLASS_LOADS_STORES          0x08000000
#define CLASS_DATA_PROCESSING_REG   0x0A000000
#define CLASS_FP_AND_SIMD          0x04000000

DecoderContext* decoder_create(const uint8_t* code, size_t size) {
    DecoderContext* ctx = (DecoderContext*)calloc(1, sizeof(DecoderContext));
    if (!ctx) return NULL;
    ctx->code_buffer = code;
    ctx->buffer_size = size;
    ctx->pc = 0;
    ctx->is_thumb_mode = false;
    return ctx;
}

void decoder_destroy(DecoderContext* context) {
    free(context);
}

uint32_t decoder_extract_bits(uint32_t instruction, uint8_t start, uint8_t length) {
    return EXTRACT_BITS(instruction, start, length);
}

static bool decode_data_processing_immediate(uint32_t inst, Instruction* decoded) {
    uint32_t op0 = decoder_extract_bits(inst, 23, 3);
    uint32_t op1 = decoder_extract_bits(inst, 30, 2);
    
    decoded->type = INST_ARITHMETIC;
    decoded->dest_reg = decoder_extract_bits(inst, 0, 5);
    
    if (op0 == 0x2) {
        decoded->opcode = (op1 & 1) ? 0x01 : 0x00;
        decoded->sets_flags = (op1 & 2) != 0;
        
        Operand imm_op = {
            .type = OP_IMMEDIATE,
            .value.immediate = decoder_extract_bits(inst, 10, 12)
        };
        instruction_set_operand(decoded, 0, imm_op);
        
        Operand reg_op = {
            .type = OP_REGISTER,
            .value.reg = decoder_extract_bits(inst, 5, 5)
        };
        instruction_set_operand(decoded, 1, reg_op);
        return true;
    }
    return false;
}

static bool decode_branches(uint32_t inst, Instruction* decoded) {
    uint32_t op0 = decoder_extract_bits(inst, 29, 3);
    decoded->type = INST_BRANCH;
    
    if (op0 == 0x0 || op0 == 0x4) {
        decoded->opcode = (op0 == 0x4) ? 0x25 : 0x20;
        int32_t imm26 = decoder_extract_bits(inst, 0, 26);
        if (imm26 & (1 << 25)) {
            imm26 |= ~((1 << 26) - 1);
        }
        Operand target = {
            .type = OP_IMMEDIATE,
            .value.immediate = imm26 << 2
        };
        instruction_set_operand(decoded, 0, target);
        return true;
    }
    
    if (op0 == 0x2) {
        decoded->opcode = 0x22;
        decoded->condition = decoder_extract_bits(inst, 0, 4);
        int32_t imm19 = decoder_extract_bits(inst, 5, 19);
        if (imm19 & (1 << 18)) {
            imm19 |= ~((1 << 19) - 1);
        }
        Operand target = {
            .type = OP_IMMEDIATE,
            .value.immediate = imm19 << 2
        };
        instruction_set_operand(decoded, 0, target);
        return true;
    }
    return false;
}

static bool decode_loads_stores(uint32_t inst, Instruction* decoded) {
    uint32_t size = decoder_extract_bits(inst, 30, 2);
    uint32_t op = decoder_extract_bits(inst, 22, 2);
    decoded->type = INST_LOAD_STORE;
    decoded->dest_reg = decoder_extract_bits(inst, 0, 5);
    
    if ((inst & 0x3B000000) == 0x39000000) {
        decoded->opcode = (op & 1) ? 0x40 : 0x41;
        Operand mem = {
            .type = OP_MEMORY,
            .value = {
                .mem = {
                    .base_reg = decoder_extract_bits(inst, 5, 5),
                    .offset = decoder_extract_bits(inst, 12, 9) << size,
                    .index_reg = 0xFF,
                    .shift_amount = 0
                }
            }
        };
        instruction_set_operand(decoded, 0, mem);
        return true;
    }
    return false;
}

bool decoder_decode_next(DecoderContext* context, Instruction* inst) {
    if (!context || !inst || context->pc >= context->buffer_size) {
        return false;
    }
    
    uint32_t raw_inst;
    memcpy(&raw_inst, context->code_buffer + context->pc, sizeof(raw_inst));
    instruction_init(inst);
    inst->raw = raw_inst;
    bool success = false;
    
    if ((raw_inst & 0x1F000000) == CLASS_DATA_PROCESSING_IMM) {
        success = decode_data_processing_immediate(raw_inst, inst);
    } else if ((raw_inst & 0x1C000000) == CLASS_BRANCHES) {
        success = decode_branches(raw_inst, inst);
    } else if ((raw_inst & 0x0A000000) == CLASS_LOADS_STORES) {
        success = decode_loads_stores(raw_inst, inst);
    }
    
    if (success) {
        context->pc += 4;
    }
    return success;
}

bool decoder_decode_at(DecoderContext* context, uint64_t address, Instruction* inst) {
    if (!context || !inst || address >= context->buffer_size) {
        return false;
    }
    uint64_t old_pc = context->pc;
    context->pc = address;
    bool result = decoder_decode_next(context, inst);
    context->pc = old_pc;
    return result;
}

size_t decoder_decode_block(DecoderContext* context, Instruction* insts, size_t max_count) {
    if (!context || !insts || !max_count) {
        return 0;
    }
    size_t count = 0;
    while (count < max_count && decoder_decode_next(context, &insts[count])) {
        count++;
        if (instruction_is_branch(&insts[count - 1])) {
            break;
        }
    }
    return count;
}

bool decoder_is_valid_instruction(uint32_t raw_instruction) {
    if ((raw_instruction & 0x1F000000) == CLASS_DATA_PROCESSING_IMM) return true;
    if ((raw_instruction & 0x1C000000) == CLASS_BRANCHES) return true;
    if ((raw_instruction & 0x0A000000) == CLASS_LOADS_STORES) return true;
    if ((raw_instruction & 0x0F000000) == CLASS_DATA_PROCESSING_REG) return true;
    if ((raw_instruction & 0x0F000000) == CLASS_FP_AND_SIMD) return true;
    return false;
}

bool decoder_can_fallthrough(const Instruction* inst) {
    if (!inst) return false;
    if (inst->type == INST_BRANCH) {
        return inst->condition != 0xF;
    }
    return true;
}

uint64_t decoder_get_next_pc(const DecoderContext* context, const Instruction* inst) {
    if (!context || !inst) return 0;
    if (!instruction_is_branch(inst)) {
        return context->pc + 4;
    }
    uint64_t target = instruction_get_branch_target(inst, context->pc);
    if (target != 0) {
        return target;
    }
    return context->pc + 4;
} 