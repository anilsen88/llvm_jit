#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/decoder.h"
#include "../include/instruction.h"

static void test_simple_instruction_decoding() {
    uint8_t mov_rax_42[] = {0x48, 0xC7, 0xC0, 0x2A, 0x00, 0x00, 0x00};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(mov_rax_42, sizeof(mov_rax_42), &instr);
    assert(bytes_read == 7);
    assert(instr.opcode == OPCODE_MOV);
    assert(instr.operand_count == 2);
    assert(instr.operands[0].type == OPERAND_REGISTER);
    assert(instr.operands[0].reg == REG_RAX);
    assert(instr.operands[1].type == OPERAND_IMMEDIATE);
    assert(instr.operands[1].immediate == 42);
}

static void test_register_to_register() {
    uint8_t mov_rdx_rax[] = {0x48, 0x89, 0xC2};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(mov_rdx_rax, sizeof(mov_rdx_rax), &instr);
    assert(bytes_read == 3);
    assert(instr.opcode == OPCODE_MOV);
    assert(instr.operand_count == 2);
    assert(instr.operands[0].type == OPERAND_REGISTER);
    assert(instr.operands[0].reg == REG_RDX);
    assert(instr.operands[1].type == OPERAND_REGISTER);
    assert(instr.operands[1].reg == REG_RAX);
}

static void test_memory_operands() {
    uint8_t mov_rax_mem[] = {0x48, 0x8B, 0x84, 0xB5, 0x80, 0xFF, 0xFF, 0xFF};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(mov_rax_mem, sizeof(mov_rax_mem), &instr);
    assert(bytes_read == 8);
    assert(instr.opcode == OPCODE_MOV);
    assert(instr.operand_count == 2);
    assert(instr.operands[0].type == OPERAND_REGISTER);
    assert(instr.operands[0].reg == REG_RAX);
    assert(instr.operands[1].type == OPERAND_MEMORY);
    assert(instr.operands[1].memory.base == REG_RBP);
    assert(instr.operands[1].memory.index == REG_RSI);
    assert(instr.operands[1].memory.scale == 4);
    assert(instr.operands[1].memory.displacement == -128);
}

static void test_arithmetic_instructions() {
    uint8_t add_rax_rbx[] = {0x48, 0x01, 0xD8};
    uint8_t sub_rcx_42[] = {0x48, 0x83, 0xE9, 0x2A};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(add_rax_rbx, sizeof(add_rax_rbx), &instr);
    assert(bytes_read == 3);
    assert(instr.opcode == OPCODE_ADD);
    assert(instr.operands[0].reg == REG_RAX);
    assert(instr.operands[1].reg == REG_RBX);
    
    bytes_read = decode_instruction(sub_rcx_42, sizeof(sub_rcx_42), &instr);
    assert(bytes_read == 4);
    assert(instr.opcode == OPCODE_SUB);
    assert(instr.operands[0].reg == REG_RCX);
    assert(instr.operands[1].immediate == 42);
}

static void test_control_flow_instructions() {
    uint8_t jmp_forward[] = {0xEB, 0x20};
    uint8_t je_backward[] = {0x74, 0xF0};
    uint8_t call_addr[] = {0xE8, 0x67, 0x45, 0x23, 0x01};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(jmp_forward, sizeof(jmp_forward), &instr);
    assert(bytes_read == 2);
    assert(instr.opcode == OPCODE_JMP);
    assert(instr.operand_count == 1);
    assert(instr.operands[0].type == OPERAND_IMMEDIATE);
    assert(instr.operands[0].immediate == 32);
    
    bytes_read = decode_instruction(je_backward, sizeof(je_backward), &instr);
    assert(bytes_read == 2);
    assert(instr.opcode == OPCODE_JE);
    assert(instr.operands[0].immediate == -16);
    
    bytes_read = decode_instruction(call_addr, sizeof(call_addr), &instr);
    assert(bytes_read == 5);
    assert(instr.opcode == OPCODE_CALL);
    assert(instr.operands[0].immediate == 0x1234567);
}

static void test_complex_instructions() {
    uint8_t imul_complex[] = {0x48, 0x6B, 0x04, 0xCB, 0x2A};
    uint8_t lea_rip[] = {0x48, 0x8D, 0x15, 0x34, 0x12, 0x00, 0x00};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(imul_complex, sizeof(imul_complex), &instr);
    assert(bytes_read == 5);
    assert(instr.opcode == OPCODE_IMUL);
    assert(instr.operand_count == 3);
    assert(instr.operands[0].reg == REG_RAX);
    assert(instr.operands[1].type == OPERAND_MEMORY);
    assert(instr.operands[1].memory.base == REG_RBX);
    assert(instr.operands[1].memory.index == REG_RCX);
    assert(instr.operands[1].memory.scale == 8);
    assert(instr.operands[2].immediate == 42);
    
    bytes_read = decode_instruction(lea_rip, sizeof(lea_rip), &instr);
    assert(bytes_read == 7);
    assert(instr.opcode == OPCODE_LEA);
    assert(instr.operands[0].reg == REG_RDX);
    assert(instr.operands[1].type == OPERAND_MEMORY);
    assert(instr.operands[1].memory.base == REG_RIP);
    assert(instr.operands[1].memory.displacement == 0x1234);
}

static void test_prefix_handling() {
    uint8_t lock_xadd[] = {0xF0, 0x48, 0x0F, 0xC1, 0x03};
    uint8_t rep_movsq[] = {0xF3, 0x48, 0xA5};
    Instruction instr;
    size_t bytes_read;
    
    bytes_read = decode_instruction(lock_xadd, sizeof(lock_xadd), &instr);
    assert(bytes_read == 5);
    assert(instr.opcode == OPCODE_XADD);
    assert(instr.flags & INSTR_FLAG_LOCK_PREFIX);
    assert(instr.operand_count == 2);
    assert(instr.operands[0].type == OPERAND_MEMORY);
    assert(instr.operands[0].memory.base == REG_RBX);
    assert(instr.operands[1].reg == REG_RAX);
    
    bytes_read = decode_instruction(rep_movsq, sizeof(rep_movsq), &instr);
    assert(bytes_read == 3);
    assert(instr.opcode == OPCODE_MOVSQ);
    assert(instr.flags & INSTR_FLAG_REP_PREFIX);
}

int main() {
    printf("Running decoder tests...\n");
    
    test_simple_instruction_decoding();
    test_register_to_register();
    test_memory_operands();
    test_arithmetic_instructions();
    test_control_flow_instructions();
    test_complex_instructions();
    test_prefix_handling();
    
    printf("All decoder tests passed!\n");
    return 0;
} 