#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

typedef enum {
    INST_UNKNOWN,
    INST_ARITHMETIC,
    INST_LOGICAL,
    INST_MOVE,
    INST_BRANCH,
    INST_LOAD_STORE,
    INST_SYSTEM,
    INST_FLOAT,
    INST_VECTOR
} InstructionType;

typedef enum {
    OP_NONE,
    OP_IMMEDIATE,
    OP_REGISTER,
    OP_MEMORY,
    OP_SHIFT,
    OP_EXTEND
} OperandType;

typedef struct {
    OperandType type;
    union {
        uint64_t immediate;
        uint8_t reg;
        struct {
            uint8_t base_reg;
            int32_t offset;
            uint8_t index_reg;
            uint8_t shift_amount;
        } mem;
    } value;
} Operand;

typedef struct Instruction {
    uint32_t raw;
    InstructionType type;
    uint8_t opcode;
    uint8_t condition;
    uint8_t dest_reg;
    Operand operands[4];
    uint8_t operand_count;
    bool sets_flags;
} Instruction;

void instruction_init(Instruction* inst);
void instruction_set_operand(Instruction* inst, uint8_t index, Operand op);
const char* instruction_to_string(const Instruction* inst, char* buffer, size_t size);
bool instruction_is_branch(const Instruction* inst);
bool instruction_is_memory_access(const Instruction* inst);
uint64_t instruction_get_branch_target(const Instruction* inst, uint64_t pc);

bool instruction_modifies_register(const Instruction* inst, uint8_t reg);
bool instruction_reads_register(const Instruction* inst, uint8_t reg);
bool instruction_can_be_reordered(const Instruction* inst);

#endif