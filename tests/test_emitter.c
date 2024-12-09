#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/emitter.h"
#include "../include/instruction.h"
#include "../include/memory.h"

static void verify_bytes(const uint8_t* actual, const uint8_t* expected, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (actual[i] != expected[i]) {
            printf("Byte mismatch at offset %zu: expected 0x%02X, got 0x%02X\n",
                   i, expected[i], actual[i]);
            assert(0);
        }
    }
}

static void test_simple_mov() {
    uint8_t buffer[15];
    size_t size;
    
    uint8_t expected_mov_rax_42[] = {0x48, 0xC7, 0xC0, 0x2A, 0x00, 0x00, 0x00};
    size = emit_mov_reg_imm(buffer, REG_RAX, 42);
    assert(size == sizeof(expected_mov_rax_42));
    verify_bytes(buffer, expected_mov_rax_42, size);
    
    uint8_t expected_mov_rdx_rax[] = {0x48, 0x89, 0xC2};
    size = emit_mov_reg_reg(buffer, REG_RDX, REG_RAX);
    assert(size == sizeof(expected_mov_rdx_rax));
    verify_bytes(buffer, expected_mov_rdx_rax, size);
}

static void test_arithmetic_operations() {
    uint8_t buffer[15];
    size_t size;
    

    uint8_t expected_add[] = {0x48, 0x01, 0xD8};
    size = emit_add_reg_reg(buffer, REG_RAX, REG_RBX);
    assert(size == sizeof(expected_add));
    verify_bytes(buffer, expected_add, size);
    
    uint8_t expected_sub[] = {0x48, 0x83, 0xE9, 0x2A};
    size = emit_sub_reg_imm(buffer, REG_RCX, 42);
    assert(size == sizeof(expected_sub));
    verify_bytes(buffer, expected_sub, size);
    
    uint8_t expected_imul[] = {0x48, 0x0F, 0xAF, 0xC3};
    size = emit_imul_reg_reg(buffer, REG_RAX, REG_RBX);
    assert(size == sizeof(expected_imul));
    verify_bytes(buffer, expected_imul, size);
}

static void test_memory_operations() {
    uint8_t buffer[15];
    size_t size;
    MemoryOperand mem;
    
    uint8_t expected_mov_mem[] = {0x48, 0x8B, 0x84, 0xB5, 0x80, 0xFF, 0xFF, 0xFF};
    mem.base = REG_RBP;
    mem.index = REG_RSI;
    mem.scale = 4;
    mem.displacement = -128;
    size = emit_mov_reg_mem(buffer, REG_RAX, &mem);
    assert(size == sizeof(expected_mov_mem));
    verify_bytes(buffer, expected_mov_mem, size);
    
    uint8_t expected_mov_mem_simple[] = {0x48, 0x89, 0x03};
    mem.base = REG_RBX;
    mem.index = REG_NONE;
    mem.scale = 0;
    mem.displacement = 0;
    size = emit_mov_mem_reg(buffer, &mem, REG_RAX);
    assert(size == sizeof(expected_mov_mem_simple));
    verify_bytes(buffer, expected_mov_mem_simple, size);
}

static void test_control_flow() {
    uint8_t buffer[15];
    size_t size;
    
    uint8_t expected_jmp[] = {0xEB, 0x20};
    size = emit_jmp_rel8(buffer, 32);
    assert(size == sizeof(expected_jmp));
    verify_bytes(buffer, expected_jmp, size);
    
    uint8_t expected_je[] = {0x74, 0xF0};
    size = emit_je_rel8(buffer, -16);
    assert(size == sizeof(expected_je));
    verify_bytes(buffer, expected_je, size);
    
    uint8_t expected_call[] = {0xE8, 0x67, 0x45, 0x23, 0x01};
    size = emit_call_rel32(buffer, 0x1234567);
    assert(size == sizeof(expected_call));
    verify_bytes(buffer, expected_call, size);
}

static void test_stack_operations() {
    uint8_t buffer[15];
    size_t size;
    
    uint8_t expected_push[] = {0x50};
    size = emit_push_reg(buffer, REG_RAX);
    assert(size == sizeof(expected_push));
    verify_bytes(buffer, expected_push, size);
    
    uint8_t expected_pop[] = {0x5B};
    size = emit_pop_reg(buffer, REG_RBX);
    assert(size == sizeof(expected_pop));
    verify_bytes(buffer, expected_pop, size);
    
    uint8_t expected_enter[] = {0xC8, 0x10, 0x00, 0x00};
    size = emit_enter(buffer, 16, 0);
    assert(size == sizeof(expected_enter));
    verify_bytes(buffer, expected_enter, size);
}

static void test_simd_operations() {
    uint8_t buffer[15];
    size_t size;
    
    uint8_t expected_movaps[] = {0x0F, 0x28, 0xC1};
    size = emit_movaps_reg_reg(buffer, REG_XMM0, REG_XMM1);
    assert(size == sizeof(expected_movaps));
    verify_bytes(buffer, expected_movaps, size);
    
    uint8_t expected_addps[] = {0x0F, 0x58, 0xC1};
    size = emit_addps_reg_reg(buffer, REG_XMM0, REG_XMM1);
    assert(size == sizeof(expected_addps));
    verify_bytes(buffer, expected_addps, size);
}

static void test_system_operations() {
    uint8_t buffer[15];
    size_t size;
    
    uint8_t expected_syscall[] = {0x0F, 0x05};
    size = emit_syscall(buffer);
    assert(size == sizeof(expected_syscall));
    verify_bytes(buffer, expected_syscall, size);
    
    uint8_t expected_cpuid[] = {0x0F, 0xA2};
    size = emit_cpuid(buffer);
    assert(size == sizeof(expected_cpuid));
    verify_bytes(buffer, expected_cpuid, size);
}

static void test_complex_sequences() {
    uint8_t buffer[64];
    size_t total_size = 0;
    size_t size;

    uint8_t expected_prologue[] = {
        0x55,                  
        0x48, 0x89, 0xE5,      
        0x48, 0x83, 0xEC, 0x20 
    };
    
    size = emit_push_reg(buffer + total_size, REG_RBP);
    total_size += size;
    size = emit_mov_reg_reg(buffer + total_size, REG_RBP, REG_RSP);
    total_size += size;
    size = emit_sub_reg_imm(buffer + total_size, REG_RSP, 32);
    total_size += size;
    
    assert(total_size == sizeof(expected_prologue));
    verify_bytes(buffer, expected_prologue, total_size);
}

int main() {
    printf("Running emitter tests...\n");
    
    test_simple_mov();
    test_arithmetic_operations();
    test_memory_operations();
    test_control_flow();
    test_stack_operations();
    test_simd_operations();
    test_system_operations();
    test_complex_sequences();
    
    printf("All emitter tests passed!\n");
    return 0;
} 