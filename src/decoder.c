#include "decoder.h"
#include <stdlib.h>
#include <string.h>

#define MASK_BITS(low, high) ((((1ULL << ((high) - (low) + 1)) - 1)) << (low))
#define EXTRACT_BITS(value, start, length) ((value >> start) & ((1ULL << length) - 1))

#define CLASS_DATA_PROCESSING_IMM   0x11000000
#define CLASS_BRANCHES              0x14000000
#define CLASS_LOADS_STORES          0x08000000
#define CLASS_DATA_PROCESSING_REG   0x0A000000
#define CLASS_FP_AND_SIMD          0x04000000

DecoderContext* decoder_create(const uint8_t* code, size_t size) {
    if (!code) return NULL;
    DecoderContext* ctx = (DecoderContext*)calloc(1, sizeof(DecoderContext));
    if (!ctx) return NULL;
    ctx->code_buffer = code;
    ctx->buffer_size = size;
    ctx->pc = 0;
    ctx->is_thumb_mode = false;
    return ctx;
}

void decoder_destroy(DecoderContext* context) {
    if (!context) return;
    
    /* Note: code_buffer is const and not owned by the decoder,
     * so we don't free it. If any dynamically allocated members
     * are added to DecoderContext in the future, free them here
     * before freeing the context itself.
     */
    
    free(context);
    /* Context is freed and should not be used after this point :) */
}

uint32_t decoder_extract_bits(uint32_t instruction, uint8_t start, uint8_t length) {
    return EXTRACT_BITS(instruction, start, length);
}

static DecoderError decode_data_processing_immediate(uint32_t inst, Instruction* decoded) {
    if (!decoded) return DECODER_ERROR_NULL_PARAM;
    
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
        return DECODER_SUCCESS;
    }
    return DECODER_ERROR_INVALID_INSTRUCTION;
}

static DecoderError decode_branches(uint32_t inst, Instruction* decoded) {
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
        return DECODER_SUCCESS;
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
        return DECODER_SUCCESS;
    }
    return DECODER_ERROR_INVALID_INSTRUCTION;
}

static DecoderError decode_loads_stores(uint32_t inst, Instruction* decoded) {
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
        return DECODER_SUCCESS;
    }
    return DECODER_ERROR_INVALID_INSTRUCTION;
}

DecoderError decoder_decode_next(DecoderContext* context, Instruction* inst) {
    if (!context || !inst) return DECODER_ERROR_NULL_PARAM;
    if (context->pc >= context->buffer_size) return DECODER_ERROR_BUFFER_OVERFLOW;
    
    uint32_t raw_inst;
    memcpy(&raw_inst, context->code_buffer + context->pc, sizeof(raw_inst));
    instruction_init(inst);
    inst->raw = raw_inst;
    DecoderError result = DECODER_ERROR_INVALID_INSTRUCTION;
    
    if ((raw_inst & 0x1F000000) == CLASS_DATA_PROCESSING_IMM) {
        result = decode_data_processing_immediate(raw_inst, inst);
    } else if ((raw_inst & 0x1C000000) == CLASS_BRANCHES) {
        result = decode_branches(raw_inst, inst);
    } else if ((raw_inst & 0x0A000000) == CLASS_LOADS_STORES) {
        result = decode_loads_stores(raw_inst, inst);
    }
    
    if (result == DECODER_SUCCESS) {
        context->pc += 4;
    }
    return result;
}

DecoderError decoder_decode_at(DecoderContext* context, uint64_t address, Instruction* inst) {
    if (!context || !inst) return DECODER_ERROR_NULL_PARAM;
    if (address >= context->buffer_size) return DECODER_ERROR_INVALID_ADDRESS;
    
    uint64_t old_pc = context->pc;
    context->pc = address;
    DecoderError result = decoder_decode_next(context, inst);
    context->pc = old_pc;
    return result;
}

DecoderError decoder_decode_block(DecoderContext* context, Instruction* insts, size_t max_count, size_t* decoded_count) {
    if (!context || !insts || !decoded_count) return DECODER_ERROR_NULL_PARAM;
    if (!max_count) return DECODER_SUCCESS;
    
    *decoded_count = 0;
    DecoderError result;
    
    while (*decoded_count < max_count) {
        result = decoder_decode_next(context, &insts[*decoded_count]);
        if (result != DECODER_SUCCESS) {
            return (*decoded_count > 0) ? DECODER_SUCCESS : result;
        }
        (*decoded_count)++;
        if (instruction_is_branch(&insts[*decoded_count - 1])) {
            break;
        }
    }
    return DECODER_SUCCESS;
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

DecoderError decoder_get_next_pc(const DecoderContext* context, const Instruction* inst, uint64_t* next_pc) {
    if (!context || !inst || !next_pc) return DECODER_ERROR_NULL_PARAM;
    
    if (!instruction_is_branch(inst)) {
        *next_pc = context->pc + 4;
        return DECODER_SUCCESS;
    }
    
    uint64_t target = instruction_get_branch_target(inst, context->pc);
    if (target == 0) {
        *next_pc = context->pc + 4;
    } else {
        *next_pc = target;
    }
    return DECODER_SUCCESS;
} 