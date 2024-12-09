#ifndef DECODER_H
#define DECODER_H

#include "instruction.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct DecoderContext {
    uint64_t pc;
    const uint8_t* code_buffer;
    size_t buffer_size;
    bool is_thumb_mode;
} DecoderContext;

DecoderContext* decoder_create(const uint8_t* code, size_t size);
void decoder_destroy(DecoderContext* context);

bool decoder_decode_next(DecoderContext* context, Instruction* inst);
bool decoder_decode_at(DecoderContext* context, uint64_t address, Instruction* inst);
size_t decoder_decode_block(DecoderContext* context, Instruction* insts, size_t max_count);

uint32_t decoder_extract_bits(uint32_t instruction, uint8_t start, uint8_t length);
bool decoder_is_valid_instruction(uint32_t raw_instruction);

bool decoder_decode_arithmetic(uint32_t raw, Instruction* inst);
bool decoder_decode_logical(uint32_t raw, Instruction* inst);
bool decoder_decode_memory(uint32_t raw, Instruction* inst);
bool decoder_decode_branch(uint32_t raw, Instruction* inst);
bool decoder_decode_system(uint32_t raw, Instruction* inst);

bool decoder_can_fallthrough(const Instruction* inst);
uint64_t decoder_get_next_pc(const DecoderContext* context, const Instruction* inst);

#endif