#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/registers.h"

static void test_register_encoding() {
    assert(get_register_encoding(REG_RAX) == 0);
    assert(get_register_encoding(REG_RCX) == 1);
    assert(get_register_encoding(REG_RDX) == 2);
    assert(get_register_encoding(REG_RBX) == 3);
    assert(get_register_encoding(REG_RSP) == 4);
    assert(get_register_encoding(REG_RBP) == 5);
    assert(get_register_encoding(REG_RSI) == 6);
    assert(get_register_encoding(REG_RDI) == 7);
    assert(get_register_encoding(REG_R8) == 8);
    assert(get_register_encoding(REG_R9) == 9);
    assert(get_register_encoding(REG_R10) == 10);
    assert(get_register_encoding(REG_R11) == 11);
    assert(get_register_encoding(REG_R12) == 12);
    assert(get_register_encoding(REG_R13) == 13);
    assert(get_register_encoding(REG_R14) == 14);
    assert(get_register_encoding(REG_R15) == 15);
}

static void test_register_types() {
    assert(is_general_purpose_register(REG_RAX));
    assert(is_general_purpose_register(REG_R15));
    assert(!is_general_purpose_register(REG_XMM0));
    assert(!is_general_purpose_register(REG_NONE));
    
    assert(is_xmm_register(REG_XMM0));
    assert(is_xmm_register(REG_XMM15));
    assert(!is_xmm_register(REG_RAX));
    assert(!is_xmm_register(REG_NONE));
}

static void test_register_names() {
    // Test register name conversion
    assert(strcmp(get_register_name(REG_RAX), "rax") == 0);
    assert(strcmp(get_register_name(REG_RCX), "rcx") == 0);
    assert(strcmp(get_register_name(REG_R8), "r8") == 0);
    assert(strcmp(get_register_name(REG_R15), "r15") == 0);
    assert(strcmp(get_register_name(REG_XMM0), "xmm0") == 0);
    assert(strcmp(get_register_name(REG_XMM15), "xmm15") == 0);
    assert(strcmp(get_register_name(REG_NONE), "none") == 0);
}

static void test_register_sizes() {
    assert(get_register_size(REG_RAX) == 8);
    assert(get_register_size(REG_EAX) == 4);
    assert(get_register_size(REG_AX) == 2);
    assert(get_register_size(REG_AL) == 1);
    assert(get_register_size(REG_XMM0) == 16);
}

static void test_register_variants() {
    assert(get_register_8bit_low(REG_RAX) == REG_AL);
    assert(get_register_8bit_high(REG_RAX) == REG_AH);
    assert(get_register_16bit(REG_RAX) == REG_AX);
    assert(get_register_32bit(REG_RAX) == REG_EAX);
    assert(get_register_64bit(REG_EAX) == REG_RAX);
    
    assert(get_register_8bit_low(REG_R8) == REG_R8B);
    assert(get_register_16bit(REG_R8) == REG_R8W);
    assert(get_register_32bit(REG_R8) == REG_R8D);
}

static void test_register_preservation() {
    assert(is_caller_saved(REG_RAX));
    assert(is_caller_saved(REG_RCX));
    assert(is_caller_saved(REG_RDX));
    assert(!is_caller_saved(REG_RBX));
    
    assert(!is_callee_saved(REG_RAX));
    assert(!is_callee_saved(REG_RCX));
    assert(!is_callee_saved(REG_RDX));
    assert(is_callee_saved(REG_RBX));
    assert(is_callee_saved(REG_RBP));
    assert(is_callee_saved(REG_R12));
    assert(is_callee_saved(REG_R13));
    assert(is_callee_saved(REG_R14));
    assert(is_callee_saved(REG_R15));
}

static void test_register_allocation() {
    RegisterAllocator* ra = register_allocator_create();
    assert(ra != NULL);

    Register reg1 = register_allocator_alloc(ra);
    assert(is_general_purpose_register(reg1));
    assert(!register_allocator_is_free(ra, reg1));
    
    Register reg2 = register_allocator_alloc(ra);
    assert(reg1 != reg2);
    assert(is_general_purpose_register(reg2));
    assert(!register_allocator_is_free(ra, reg2));

    register_allocator_free(ra, reg1);
    assert(register_allocator_is_free(ra, reg1));
    
    Register reg3 = register_allocator_alloc(ra);
    assert(reg3 == reg1);
    
    register_allocator_destroy(ra);
}

static void test_register_spilling() {
    RegisterAllocator* ra = register_allocator_create();

    Register allocated[MAX_GP_REGISTERS];
    int count = 0;
    
    while (count < MAX_GP_REGISTERS) {
        Register reg = register_allocator_alloc(ra);
        if (reg == REG_NONE) break;
        allocated[count++] = reg;
    }
    
    assert(count == MAX_GP_REGISTERS);
    
    Register spilled = register_allocator_alloc_with_spill(ra);
    assert(spilled != REG_NONE);
    
    for (int i = 0; i < count; i++) {
        register_allocator_free(ra, allocated[i]);
    }
    register_allocator_free(ra, spilled);
    
    register_allocator_destroy(ra);
}

static void test_register_constraints() {
    RegisterAllocator* ra = register_allocator_create();
    
    RegisterConstraint constraint = {
        .allowed_registers = (1ULL << REG_RAX) | (1ULL << REG_RBX) | (1ULL << REG_RCX),
        .preferred_register = REG_RAX
    };
    
    Register reg = register_allocator_alloc_constrained(ra, &constraint);
    assert(reg == REG_RAX);
    
    Register reg2 = register_allocator_alloc_constrained(ra, &constraint);
    assert(reg2 == REG_RBX || reg2 == REG_RCX);
    
    register_allocator_free(ra, reg);
    register_allocator_free(ra, reg2);
    register_allocator_destroy(ra);
}

int main() {
    printf("Running register tests...\n");
    
    test_register_encoding();
    test_register_types();
    test_register_names();
    test_register_sizes();
    test_register_variants();
    test_register_preservation();
    test_register_allocation();
    test_register_spilling();
    test_register_constraints();
    
    printf("All register tests passed!\n");
    return 0;
} 