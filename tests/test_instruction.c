#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/instruction.h"
#include "../include/decoder.h"

static void test_instruction_creation() {
    Instruction instr;
    instr_init(&instr, OPCODE_MOV, 0);
    assert(instr.opcode == OPCODE_MOV);
    assert(instr.flags == 0);
    assert(instr.operand_count == 0);
}

static void test_instruction_operands() {
    Instruction instr;
    instr_init(&instr, OPCODE_ADD, 0);
    
    Operand op1 = {.type = OPERAND_REGISTER, .reg = REG_RAX};
    Operand op2 = {.type = OPERAND_IMMEDIATE, .immediate = 42};
    
    instr_add_operand(&instr, &op1);
    instr_add_operand(&instr, &op2);
    
    assert(instr.operand_count == 2);
    assert(instr.operands[0].type == OPERAND_REGISTER);
    assert(instr.operands[0].reg == REG_RAX);
    assert(instr.operands[1].type == OPERAND_IMMEDIATE);
    assert(instr.operands[1].immediate == 42);
}

static void test_instruction_flags() {
    Instruction instr;
    instr_init(&instr, OPCODE_MOV, INSTR_FLAG_REX_W);
    
    assert(instr.flags & INSTR_FLAG_REX_W);
    assert(!(instr.flags & INSTR_FLAG_MODRM));
    
    instr_add_flag(&instr, INSTR_FLAG_MODRM);
    assert(instr.flags & INSTR_FLAG_MODRM);
}

static void test_instruction_memory_operands() {
    Instruction instr;
    instr_init(&instr, OPCODE_MOV, 0);
    
    MemoryOperand mem = {
        .base = REG_RBP,
        .index = REG_RSI,
        .scale = 4,
        .displacement = -128
    };
    
    Operand op = {.type = OPERAND_MEMORY, .memory = mem};
    instr_add_operand(&instr, &op);
    
    assert(instr.operand_count == 1);
    assert(instr.operands[0].type == OPERAND_MEMORY);
    assert(instr.operands[0].memory.base == REG_RBP);
    assert(instr.operands[0].memory.index == REG_RSI);
    assert(instr.operands[0].memory.scale == 4);
    assert(instr.operands[0].memory.displacement == -128);
}

static void test_instruction_validation() {
    Instruction instr;
    
    // Test valid instruction
    instr_init(&instr, OPCODE_MOV, 0);
    Operand dst = {.type = OPERAND_REGISTER, .reg = REG_RAX};
    Operand src = {.type = OPERAND_REGISTER, .reg = REG_RBX};
    instr_add_operand(&instr, &dst);
    instr_add_operand(&instr, &src);
    assert(instr_validate(&instr) == 1);
    
    instr_init(&instr, OPCODE_RET, 0);
    instr_add_operand(&instr, &dst);
    assert(instr_validate(&instr) == 0);
}

static void test_instruction_encoding() {
    Instruction instr;
    uint8_t buffer[15];
    size_t size;
    
    instr_init(&instr, OPCODE_MOV, INSTR_FLAG_REX_W);
    Operand dst = {.type = OPERAND_REGISTER, .reg = REG_RAX};
    Operand src = {.type = OPERAND_IMMEDIATE, .immediate = 42};
    instr_add_operand(&instr, &dst);
    instr_add_operand(&instr, &src);
    
    size = instr_encode(&instr, buffer);
    assert(size > 0);

    instr_init(&instr, OPCODE_ADD, INSTR_FLAG_REX_W | INSTR_FLAG_MODRM);
    dst.reg = REG_RAX;
    src.type = OPERAND_REGISTER;
    src.reg = REG_RBX;
    instr_add_operand(&instr, &dst);
    instr_add_operand(&instr, &src);
    
    size = instr_encode(&instr, buffer);
    assert(size > 0);
}

static void test_instruction_string() {
    Instruction instr;
    char buffer[256];
    
    instr_init(&instr, OPCODE_MOV, INSTR_FLAG_REX_W);
    Operand dst = {.type = OPERAND_REGISTER, .reg = REG_RAX};
    Operand src = {.type = OPERAND_IMMEDIATE, .immediate = 42};
    instr_add_operand(&instr, &dst);
    instr_add_operand(&instr, &src);
    
    instr_to_string(&instr, buffer, sizeof(buffer));
    assert(strstr(buffer, "MOV") != NULL);
    assert(strstr(buffer, "RAX") != NULL);
    assert(strstr(buffer, "42") != NULL);
}

int main() {
    printf("Running instruction tests...\n");
    
    test_instruction_creation();
    test_instruction_operands();
    test_instruction_flags();
    test_instruction_memory_operands();
    test_instruction_validation();
    test_instruction_encoding();
    test_instruction_string();
    
    printf("All instruction tests passed!\n");
    return 0;
} 