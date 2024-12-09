#include "jit.h"
#include "decoder.h"
#include "emitter.h"
#include "memory.h"
#include "registers.h"
#include <stdlib.h>
#include <string.h>

#define MAX_BLOCK_SIZE 1024
#define INITIAL_CACHE_SIZE 1024

typedef struct {
    uint64_t address;
    void* code;
} CacheEntry;

static void initialize_llvm(void) {
    static bool initialized = false;
    if (!initialized) {
        LLVMInitializeCore(LLVMGetGlobalPassRegistry());
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        initialized = true;
    }
}

JITContext* jit_create(Memory* memory, RegisterFile* registers) {
    if (!memory || !registers) return NULL;
    
    initialize_llvm();
    
    JITContext* ctx = (JITContext*)calloc(1, sizeof(JITContext));
    if (!ctx) return NULL;
    
    ctx->llvm_context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext("jit_module", ctx->llvm_context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->llvm_context);
    
    char* error = NULL;
    LLVMMCJITCompilerOptions options;
    LLVMInitializeMCJITCompilerOptions(&options, sizeof(options));
    if (LLVMCreateMCJITCompilerForModule(&ctx->engine, ctx->module, &options,
                                        sizeof(options), &error) != 0) {
        fprintf(stderr, "Failed to create JIT: %s\n", error);
        LLVMDisposeMessage(error);
        jit_destroy(ctx);
        return NULL;
    }
    
    ctx->pass_manager = LLVMCreatePassManager();
    ctx->compiled_blocks = (void**)calloc(INITIAL_CACHE_SIZE, sizeof(void*));
    if (!ctx->compiled_blocks) {
        jit_destroy(ctx);
        return NULL;
    }
    ctx->cache_size = INITIAL_CACHE_SIZE;
    ctx->memory = memory;
    ctx->registers = registers;
    
    return ctx;
}

void jit_destroy(JITContext* context) {
    if (!context) return;
    
    if (context->compiled_blocks) {
        for (size_t i = 0; i < context->cache_size; i++) {
            if (context->compiled_blocks[i]) {
                context->compiled_blocks[i] = NULL;
            }
        }
        free(context->compiled_blocks);
    }
    
    if (context->pass_manager) LLVMDisposePassManager(context->pass_manager);
    if (context->builder) LLVMDisposeBuilder(context->builder);
    if (context->engine) LLVMDisposeExecutionEngine(context->engine);
    if (context->llvm_context) LLVMContextDispose(context->llvm_context);
    
    free(context);
}

void jit_add_basic_optimizations(JITContext* context) {
    if (!context || !context->pass_manager) return;
    
    LLVMAddPromoteMemoryToRegisterPass(context->pass_manager);
    LLVMAddInstructionCombiningPass(context->pass_manager);
    LLVMAddReassociatePass(context->pass_manager);
    LLVMAddGVNPass(context->pass_manager);
    LLVMAddCFGSimplificationPass(context->pass_manager);
}

void jit_optimize_block(JITContext* context, LLVMValueRef function) {
    if (!context || !function) return;
    LLVMRunPassManager(context->pass_manager, function);
}

static bool compile_block(JITContext* context, uint64_t address,
                        DecoderContext* decoder, EmitterContext* emitter) {
    if (!emitter_create_entry_block(emitter)) {
        return false;
    }
    
    Instruction inst;
    size_t count = 0;
    bool success = true;
    
    while (count < MAX_BLOCK_SIZE) {
        if (!decoder_decode_at(decoder, address + count * 4, &inst)) {
            break;
        }
        
        if (!emitter_emit_instruction(emitter, &inst)) {
            success = false;
            break;
        }
        
        count++;
        
        if (instruction_is_branch(&inst)) {
            break;
        }
    }
    
    if (!success || count == 0) {
        return false;
    }
    
    LLVMValueRef function = emitter_finalize_block(emitter);
    if (!function) {
        return false;
    }
    
    jit_optimize_block(context, function);
    
    char* error = NULL;
    if (LLVMVerifyFunction(function, LLVMPrintMessageAction, &error) != 0) {
        fprintf(stderr, "Function verification failed: %s\n", error);
        LLVMDisposeMessage(error);
        return false;
    }
    
    return true;
}

void* jit_compile_block(JITContext* context, uint64_t address) {
    if (!context || !context->memory) return NULL;
    
    void* cached = jit_get_cached_block(context, address);
    if (cached) return cached;
    
    uint8_t* code_ptr = (uint8_t*)context->memory + address;
    size_t size = context->memory->total_mapped_size - address;
    
    DecoderContext* decoder = decoder_create(code_ptr, size);
    EmitterContext* emitter = emitter_create(context);
    
    if (!decoder || !emitter) {
        decoder_destroy(decoder);
        emitter_destroy(emitter);
        return NULL;
    }
    
    bool success = compile_block(context, address, decoder, emitter);
    void* function_ptr = NULL;
    
    if (success) {
        function_ptr = LLVMGetPointerToGlobal(context->engine, emitter->function);
        if (function_ptr) {
            jit_cache_compiled_block(context, address, function_ptr);
        }
    }
    
    decoder_destroy(decoder);
    emitter_destroy(emitter);
    
    return function_ptr;
}

bool jit_execute_block(JITContext* context, void* block) {
    if (!context || !block) return false;
    
    typedef uint64_t (*BlockFunction)(void* cpu_state, void* memory_context);
    BlockFunction func = (BlockFunction)block;
    uint64_t next_pc = func(context->registers, context->memory);
    registers_set_pc(context->registers, next_pc);
    
    return true;
}

void jit_cache_compiled_block(JITContext* context, uint64_t address, void* block) {
    if (!context || !block) return;
    size_t index = (address / 4) % context->cache_size;
    context->compiled_blocks[index] = block;
}

void* jit_get_cached_block(JITContext* context, uint64_t address) {
    if (!context) return NULL;
    size_t index = (address / 4) % context->cache_size;
    return context->compiled_blocks[index];
}

void jit_invalidate_cache(JITContext* context, uint64_t address) {
    if (!context) return;
    size_t index = (address / 4) % context->cache_size;
    context->compiled_blocks[index] = NULL;
} 