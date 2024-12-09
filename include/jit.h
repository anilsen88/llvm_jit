#ifndef JIT_H
#define JIT_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <stdint.h>
#include <stdbool.h>

struct Instruction;
struct Memory;
struct RegisterFile;

typedef struct JITContext {
    LLVMContextRef llvm_context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMExecutionEngineRef engine;
    LLVMPassManagerRef pass_manager;
    
    void** compiled_blocks;
    size_t cache_size;
    
    struct Memory* memory;
    struct RegisterFile* registers;
} JITContext;

JITContext* jit_create(struct Memory* memory, struct RegisterFile* registers);
void jit_destroy(JITContext* context);

void* jit_compile_block(JITContext* context, uint64_t address);
bool jit_execute_block(JITContext* context, void* block);
void jit_invalidate_cache(JITContext* context, uint64_t address);

void jit_optimize_block(JITContext* context, LLVMValueRef function);
void jit_add_basic_optimizations(JITContext* context);

void jit_cache_compiled_block(JITContext* context, uint64_t address, void* block);
void* jit_get_cached_block(JITContext* context, uint64_t address);

#endif // JIT_H 