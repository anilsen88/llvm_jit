#ifndef EMITTER_H
#define EMITTER_H

#include "instruction.h"
#include "jit.h"
#include <llvm-c/Core.h>
#include <stdbool.h>

typedef struct EmitterContext {
    JITContext* jit;
    LLVMBasicBlockRef current_block;
    LLVMValueRef function;
    LLVMValueRef* register_values;
    LLVMValueRef* vector_registers;
    LLVMValueRef flag_n;
    LLVMValueRef flag_z;
    LLVMValueRef flag_c;
    LLVMValueRef flag_v;
} EmitterContext;

EmitterContext* emitter_create(JITContext* jit);
void emitter_destroy(EmitterContext* context);

bool emitter_emit_instruction(EmitterContext* context, const Instruction* inst);
LLVMValueRef emitter_finalize_block(EmitterContext* context);

LLVMValueRef emitter_get_register(EmitterContext* context, uint8_t reg);
void emitter_set_register(EmitterContext* context, uint8_t reg, LLVMValueRef value);
void emitter_update_flags(EmitterContext* context, LLVMValueRef result, bool update_overflow);

bool emitter_emit_arithmetic(EmitterContext* context, const Instruction* inst);
bool emitter_emit_logical(EmitterContext* context, const Instruction* inst);
bool emitter_emit_memory(EmitterContext* context, const Instruction* inst);
bool emitter_emit_branch(EmitterContext* context, const Instruction* inst);
bool emitter_emit_move(EmitterContext* context, const Instruction* inst);

LLVMValueRef emitter_create_entry_block(EmitterContext* context);
void emitter_create_exit_block(EmitterContext* context);
LLVMValueRef emitter_get_condition_value(EmitterContext* context, uint8_t condition);

#endif