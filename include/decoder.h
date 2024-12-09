#ifndef DECODER_H
#define DECODER_H

#include "instruction.h"
#include <stdint.h>
#include <stdbool.h>

/* Error codes for decoder functions
 * Negative values indicate errors
 * Zero or positive values indicate success
 */
typedef enum DecoderError {
    DECODER_SUCCESS = 0,
    DECODER_ERROR_NULL_PARAM = -1,
    DECODER_ERROR_INVALID_ADDRESS = -2,
    DECODER_ERROR_BUFFER_OVERFLOW = -3,
    DECODER_ERROR_INVALID_INSTRUCTION = -4,
    DECODER_ERROR_OUT_OF_MEMORY = -5
} DecoderError;

typedef struct DecoderContext {
    uint64_t pc;
    const uint8_t* code_buffer;
    size_t buffer_size;
    bool is_thumb_mode;
} DecoderContext;

/* Returns NULL on allocation failure */
DecoderContext* decoder_create(const uint8_t* code, size_t size);
void decoder_destroy(DecoderContext* context);

/* All decoder functions return DecoderError:
 * - Negative values indicate specific errors (see enum above)
 * - Zero indicates success
 * - Positive values indicate success with additional info (e.g., number of decoded instructions)
 */
DecoderError decoder_decode_next(DecoderContext* context, Instruction* inst);
DecoderError decoder_decode_at(DecoderContext* context, uint64_t address, Instruction* inst);
DecoderError decoder_decode_block(DecoderContext* context, Instruction* insts, size_t max_count, size_t* decoded_count);

/* Utility functions - these don't fail so they keep their return types */
uint32_t decoder_extract_bits(uint32_t instruction, uint8_t start, uint8_t length);
bool decoder_is_valid_instruction(uint32_t raw_instruction);

/* Instruction type decoders */
DecoderError decoder_decode_arithmetic(uint32_t raw, Instruction* inst);
DecoderError decoder_decode_logical(uint32_t raw, Instruction* inst);
DecoderError decoder_decode_memory(uint32_t raw, Instruction* inst);
DecoderError decoder_decode_branch(uint32_t raw, Instruction* inst);
DecoderError decoder_decode_system(uint32_t raw, Instruction* inst);

/* Control flow analysis */
bool decoder_can_fallthrough(const Instruction* inst);
DecoderError decoder_get_next_pc(const DecoderContext* context, const Instruction* inst, uint64_t* next_pc);

#endif