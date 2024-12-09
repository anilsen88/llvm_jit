#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/jit.h"
#include "../include/memory.h"
#include "../include/registers.h"

static void test_jit_initialization() {
    JITContext* ctx = jit_create_context();
    assert(ctx != NULL);
    assert(ctx->code_buffer != NULL);
    assert(ctx->code_size == 0);
    jit_destroy_context(ctx);
}

static void test_simple_function_compilation() {
    JITContext* ctx = jit_create_context();
    uint64_t (*func)(void);
    
    jit_begin_function(ctx);
    jit_emit_mov_immediate(ctx, REG_RAX, 42);
    jit_emit_ret(ctx);
    func = (uint64_t (*)(void))jit_end_function(ctx);
    
    uint64_t result = func();
    assert(result == 42);
    jit_destroy_context(ctx);
}

static void test_arithmetic_operations() {
    JITContext* ctx = jit_create_context();
    uint64_t (*func)(uint64_t, uint64_t);
    
    jit_begin_function(ctx);
    jit_emit_mov_reg(ctx, REG_RAX, REG_RDI);
    jit_emit_add_reg(ctx, REG_RAX, REG_RSI);
    jit_emit_mul_immediate(ctx, REG_RAX, 2);
    jit_emit_ret(ctx);
    func = (uint64_t (*)(uint64_t, uint64_t))jit_end_function(ctx);
    
    assert(func(3, 4) == 14);
    assert(func(10, 5) == 30);
    jit_destroy_context(ctx);
}

static void test_conditional_branching() {
    JITContext* ctx = jit_create_context();
    uint64_t (*func)(uint64_t);
    
    jit_begin_function(ctx);
    JITLabel* else_label = jit_create_label(ctx);
    JITLabel* end_label = jit_create_label(ctx);
    
    jit_emit_cmp_immediate(ctx, REG_RDI, 10);
    jit_emit_jge(ctx, else_label);
    
    jit_emit_mov_immediate(ctx, REG_RAX, 1);
    jit_emit_jmp(ctx, end_label);
    
    jit_bind_label(ctx, else_label);
    jit_emit_mov_immediate(ctx, REG_RAX, 2);
    
    jit_bind_label(ctx, end_label);
    jit_emit_ret(ctx);
    
    func = (uint64_t (*)(uint64_t))jit_end_function(ctx);
    
    assert(func(5) == 1);
    assert(func(15) == 2);
    jit_destroy_context(ctx);
}

static void test_loop_generation() {
    JITContext* ctx = jit_create_context();
    uint64_t (*func)(uint64_t);
    
    jit_begin_function(ctx);
    JITLabel* loop_start = jit_create_label(ctx);
    JITLabel* loop_end = jit_create_label(ctx);
    
    jit_emit_mov_immediate(ctx, REG_RAX, 0);
    jit_emit_mov_reg(ctx, REG_RCX, REG_RDI);
    
    jit_bind_label(ctx, loop_start);
    jit_emit_cmp_immediate(ctx, REG_RCX, 0);
    jit_emit_je(ctx, loop_end);
    
    jit_emit_add_immediate(ctx, REG_RAX, 1);
    jit_emit_sub_immediate(ctx, REG_RCX, 1);
    jit_emit_jmp(ctx, loop_start);
    
    jit_bind_label(ctx, loop_end);
    jit_emit_ret(ctx);
    
    func = (uint64_t (*)(uint64_t))jit_end_function(ctx);
    
    assert(func(5) == 5);
    assert(func(10) == 10);
    assert(func(0) == 0);
    jit_destroy_context(ctx);
}

static void test_function_calls() {
    JITContext* ctx = jit_create_context();
    uint64_t (*func)(uint64_t);
    
    jit_begin_function(ctx);
    jit_emit_push(ctx, REG_RBP);
    jit_emit_mov_reg(ctx, REG_RBP, REG_RSP);
    
    jit_emit_sub_immediate(ctx, REG_RSP, 32);
    jit_emit_mov_reg(ctx, REG_RCX, REG_RDI);
    jit_emit_call(ctx, (void*)printf);
    
    jit_emit_mov_reg(ctx, REG_RSP, REG_RBP);
    jit_emit_pop(ctx, REG_RBP);
    jit_emit_ret(ctx);
    
    func = (uint64_t (*)(uint64_t))jit_end_function(ctx);
    jit_destroy_context(ctx);
}

int main() {
    printf("Running JIT compiler tests...\n");
    
    test_jit_initialization();
    test_simple_function_compilation();
    test_arithmetic_operations();
    test_conditional_branching();
    test_loop_generation();
    test_function_calls();
    
    printf("All JIT compiler tests passed!\n");
    return 0;
} 