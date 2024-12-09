#include "emitter.h"
#include <stdlib.h>
#include <string.h>

static LLVMTypeRef get_int1_type(EmitterContext* ctx) {
    return LLVMInt1TypeInContext(ctx->jit->llvm_context);
}

static LLVMTypeRef get_int8_type(EmitterContext* ctx) {
    return LLVMInt8TypeInContext(ctx->jit->llvm_context);
}

static LLVMTypeRef get_int32_type(EmitterContext* ctx) {
    return LLVMInt32TypeInContext(ctx->jit->llvm_context);
}

static LLVMTypeRef get_int64_type(EmitterContext* ctx) {
    return LLVMInt64TypeInContext(ctx->jit->llvm_context);
}

EmitterContext* emitter_create(JITContext* jit) {
    if (!jit) return NULL;
    
    EmitterContext* ctx = (EmitterContext*)calloc(1, sizeof(EmitterContext));
    if (!ctx) return NULL;
    
    ctx->jit = jit;
    ctx->register_values = (LLVMValueRef*)calloc(64, sizeof(LLVMValueRef));
    ctx->vector_registers = (LLVMValueRef*)calloc(32, sizeof(LLVMValueRef));
    
    if (!ctx->register_values || !ctx->vector_registers) {
        free(ctx->register_values);
        free(ctx->vector_registers);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

void emitter_destroy(EmitterContext* context) {
    if (!context) return;
    free(context->register_values);
    free(context->vector_registers);
    free(context);
}

LLVMValueRef emitter_get_register(EmitterContext* context, uint8_t reg) {
    if (!context || reg >= 64) return NULL;
    return context->register_values[reg];
}

void emitter_set_register(EmitterContext* context, uint8_t reg, LLVMValueRef value) {
    if (!context || reg >= 64) return;
    context->register_values[reg] = value;
}

static LLVMValueRef create_memory_access_function(EmitterContext* ctx, bool is_store, int size) {
    LLVMTypeRef param_types[] = {
        LLVMPointerType(get_int8_type(ctx), 0),
        get_int64_type(ctx),
        is_store ? get_int64_type(ctx) : NULL
    };
    
    LLVMTypeRef return_type = is_store ? get_int1_type(ctx) : get_int64_type(ctx);
    int param_count = is_store ? 3 : 2;
    
    LLVMTypeRef func_type = LLVMFunctionType(return_type, param_types, param_count, false);
    const char* func_name = is_store ? 
        (size == 8 ? "memory_write64" : "memory_write32") :
        (size == 8 ? "memory_read64" : "memory_read32");
    
    return LLVMAddFunction(ctx->jit->module, func_name, func_type);
}

static LLVMValueRef emit_memory_access(EmitterContext* ctx, uint64_t address, 
                                     LLVMValueRef value, bool is_store, int size) {
    LLVMValueRef func = create_memory_access_function(ctx, is_store, size);
    if (!func) return NULL;
    
    LLVMValueRef args[] = {
        LLVMBuildBitCast(ctx->jit->builder, 
                         ctx->jit->memory,
                         LLVMPointerType(get_int8_type(ctx), 0),
                         "memory_ctx"),
        LLVMConstInt(get_int64_type(ctx), address, false),
        value
    };
    
    return LLVMBuildCall2(ctx->jit->builder, func, args, is_store ? 3 : 2, "");
}

void emitter_update_flags(EmitterContext* context, LLVMValueRef result, bool update_overflow) {
    if (!context || !result) return;
    
    LLVMBuilderRef builder = context->jit->builder;
    
    context->flag_n = LLVMBuildICmp(builder, LLVMIntSLT,
                                   result,
                                   LLVMConstInt(get_int64_type(context), 0, false),
                                   "flag_n");
    
    context->flag_z = LLVMBuildICmp(builder, LLVMIntEQ,
                                   result,
                                   LLVMConstInt(get_int64_type(context), 0, false),
                                   "flag_z");
    
    if (update_overflow) {
        context->flag_c = LLVMConstInt(get_int1_type(context), 0, false);
        context->flag_v = LLVMConstInt(get_int1_type(context), 0, false);
    }
}

bool emitter_emit_arithmetic(EmitterContext* context, const Instruction* inst) {
    if (!context || !inst) return false;
    
    LLVMBuilderRef builder = context->jit->builder;
    LLVMValueRef op1 = emitter_get_register(context, inst->operands[0].value.reg);
    LLVMValueRef op2;
    
    if (inst->operands[1].type == OP_IMMEDIATE) {
        op2 = LLVMConstInt(get_int64_type(context),
                          inst->operands[1].value.immediate,
                          false);
    } else {
        op2 = emitter_get_register(context, inst->operands[1].value.reg);
    }
    
    LLVMValueRef result;
    switch (inst->opcode) {
        case 0x00:
            result = LLVMBuildAdd(builder, op1, op2, "add");
            break;
        case 0x01:
            result = LLVMBuildSub(builder, op1, op2, "sub");
            break;
        default:
            return false;
    }
    
    emitter_set_register(context, inst->dest_reg, result);
    
    if (inst->sets_flags) {
        emitter_update_flags(context, result, true);
    }
    
    return true;
}

bool emitter_emit_logical(EmitterContext* context, const Instruction* inst) {
    if (!context || !inst) return false;
    
    LLVMBuilderRef builder = context->jit->builder;
    LLVMValueRef op1 = emitter_get_register(context, inst->operands[0].value.reg);
    LLVMValueRef op2;
    
    if (inst->operands[1].type == OP_IMMEDIATE) {
        op2 = LLVMConstInt(get_int64_type(context),
                          inst->operands[1].value.immediate,
                          false);
    } else {
        op2 = emitter_get_register(context, inst->operands[1].value.reg);
    }
    
    LLVMValueRef result;
    switch (inst->opcode) {
        case 0x10:
            result = LLVMBuildAnd(builder, op1, op2, "and");
            break;
        case 0x11:
            result = LLVMBuildOr(builder, op1, op2, "orr");
            break;
        case 0x12:
            result = LLVMBuildXor(builder, op1, op2, "eor");
            break;
        default:
            return false;
    }
    
    emitter_set_register(context, inst->dest_reg, result);
    
    if (inst->sets_flags) {
        emitter_update_flags(context, result, false);
    }
    
    return true;
}

bool emitter_emit_memory(EmitterContext* context, const Instruction* inst) {
    if (!context || !inst) return false;
    
    LLVMBuilderRef builder = context->jit->builder;
    LLVMValueRef base = emitter_get_register(context, inst->operands[0].value.mem.base_reg);
    LLVMValueRef offset = LLVMConstInt(get_int64_type(context),
                                     inst->operands[0].value.mem.offset,
                                     true);
    LLVMValueRef addr = LLVMBuildAdd(builder, base, offset, "addr");
    
    switch (inst->opcode) {
        case 0x40: {
            LLVMValueRef value = emit_memory_access(context, 0, NULL, false, 8);
            if (!value) return false;
            emitter_set_register(context, inst->dest_reg, value);
            break;
        }
        case 0x41: {
            LLVMValueRef value = emitter_get_register(context, inst->dest_reg);
            if (!emit_memory_access(context, 0, value, true, 8)) {
                return false;
            }
            break;
        }
        default:
            return false;
    }
    
    return true;
}

bool emitter_emit_branch(EmitterContext* context, const Instruction* inst) {
    if (!context || !inst) return false;
    
    LLVMBuilderRef builder = context->jit->builder;
    LLVMBasicBlockRef target_block = LLVMAppendBasicBlock(context->function, "target");
    LLVMBasicBlockRef next_block = LLVMAppendBasicBlock(context->function, "next");
    
    switch (inst->opcode) {
        case 0x20:
            LLVMBuildBr(builder, target_block);
            break;
            
        case 0x22: {
            LLVMValueRef condition = emitter_get_condition_value(context, inst->condition);
            LLVMBuildCondBr(builder, condition, target_block, next_block);
            break;
        }
            
        case 0x25: {
            LLVMValueRef return_addr = LLVMConstInt(get_int64_type(context),
                                                  inst->operands[0].value.immediate + 4,
                                                  false);
            emitter_set_register(context, 30, return_addr);
            LLVMBuildBr(builder, target_block);
            break;
        }
            
        default:
            return false;
    }
    
    return true;
}

LLVMValueRef emitter_get_condition_value(EmitterContext* context, uint8_t condition) {
    if (!context) return NULL;
    
    LLVMBuilderRef builder = context->jit->builder;
    
    switch (condition) {
        case 0x0:
            return context->flag_z;
        case 0x1:
            return LLVMBuildNot(builder, context->flag_z, "ne");
        case 0x2:
            return context->flag_c;
        case 0x3:
            return LLVMBuildNot(builder, context->flag_c, "cc");
        case 0x4:
            return context->flag_n;
        case 0x5:
            return LLVMBuildNot(builder, context->flag_n, "pl");
        case 0x6:
            return context->flag_v;
        case 0x7:
            return LLVMBuildNot(builder, context->flag_v, "vc");
        default:
            return LLVMConstInt(get_int1_type(context), 1, false);
    }
}

LLVMValueRef emitter_create_entry_block(EmitterContext* context) {
    if (!context) return NULL;
    
    LLVMTypeRef param_types[] = {
        LLVMPointerType(get_int8_type(context), 0),
        LLVMPointerType(get_int8_type(context), 0)
    };
    
    LLVMTypeRef func_type = LLVMFunctionType(get_int64_type(context),
                                            param_types, 2, false);
    
    context->function = LLVMAddFunction(context->jit->module, "block", func_type);
    context->current_block = LLVMAppendBasicBlock(context->function, "entry");
    LLVMPositionBuilderAtEnd(context->jit->builder, context->current_block);
    
    return context->function;
}

void emitter_create_exit_block(EmitterContext* context) {
    if (!context) return;
    
    LLVMBuilderRef builder = context->jit->builder;
    LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(context->function, "exit");
    LLVMPositionBuilderAtEnd(builder, exit_block);
    
    LLVMValueRef next_pc = emitter_get_register(context, 32);
    LLVMBuildRet(builder, next_pc);
}

bool emitter_emit_instruction(EmitterContext* context, const Instruction* inst) {
    if (!context || !inst) return false;
    
    switch (inst->type) {
        case INST_ARITHMETIC:
            return emitter_emit_arithmetic(context, inst);
        case INST_LOGICAL:
            return emitter_emit_logical(context, inst);
        case INST_LOAD_STORE:
            return emitter_emit_memory(context, inst);
        case INST_BRANCH:
            return emitter_emit_branch(context, inst);
        default:
            return false;
    }
}

LLVMValueRef emitter_finalize_block(EmitterContext* context) {
    if (!context) return NULL;
    
    emitter_create_exit_block(context);
    return context->function;
} 