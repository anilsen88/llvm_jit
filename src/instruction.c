#include "instruction.h"
#include <string.h>
#include <stdio.h>

void instruction_init(Instruction* inst) {
    if (!inst) return;
    memset(inst, 0, sizeof(Instruction));
}

void instruction_set_operand(Instruction* inst, uint8_t index, Operand op) {
    if (!inst || index >= 4) return;
    inst->operands[index] = op;
    if (index >= inst->operand_count) {
        inst->operand_count = index + 1;
    }
}

const char* instruction_to_string(const Instruction* inst, char* buffer, size_t size) {
    if (!inst || !buffer || !size) return NULL;
    
    const char* type_str;
    switch (inst->type) {
        case INST_ARITHMETIC: type_str = "ARITH"; break;
        case INST_LOGICAL:    type_str = "LOGIC"; break;
        case INST_MOVE:       type_str = "MOVE"; break;
        case INST_BRANCH:     type_str = "BRANCH"; break;
        case INST_LOAD_STORE: type_str = "MEM"; break;
        case INST_SYSTEM:     type_str = "SYS"; break;
        case INST_FLOAT:      type_str = "FLOAT"; break;
        case INST_VECTOR:     type_str = "VECTOR"; break;
        default:             type_str = "UNKNOWN"; break;
    }
    
    int written = snprintf(buffer, size, "%s [%02X] ", type_str, inst->opcode);
    if (written < 0 || written >= size) return buffer;
    
    if (inst->dest_reg != 0xFF) {
        written += snprintf(buffer + written, size - written, "dst=X%d ", inst->dest_reg);
        if (written < 0 || written >= size) return buffer;
    }
    
    for (uint8_t i = 0; i < inst->operand_count; i++) {
        const Operand* op = &inst->operands[i];
        switch (op->type) {
            case OP_IMMEDIATE:
                written += snprintf(buffer + written, size - written, "#0x%lx", op->value.immediate);
                break;
            case OP_REGISTER:
                written += snprintf(buffer + written, size - written, "X%d", op->value.reg);
                break;
            case OP_MEMORY:
                written += snprintf(buffer + written, size - written, "[X%d, #%d]", 
                                 op->value.mem.base_reg, op->value.mem.offset);
                break;
            case OP_SHIFT:
                written += snprintf(buffer + written, size - written, "LSL #%d", 
                                 op->value.mem.shift_amount);
                break;
            case OP_EXTEND:
                written += snprintf(buffer + written, size - written, "EXTEND");
                break;
            default:
                written += snprintf(buffer + written, size - written, "???");
                break;
        }
        
        if (i < inst->operand_count - 1) {
            written += snprintf(buffer + written, size - written, ", ");
        }
        
        if (written < 0 || written >= size) break;
    }
    
    if (inst->sets_flags) {
        written += snprintf(buffer + written, size - written, " [FLAGS]");
    }
    
    return buffer;
}

bool instruction_is_branch(const Instruction* inst) {
    return inst && inst->type == INST_BRANCH;
}

bool instruction_is_memory_access(const Instruction* inst) {
    return inst && inst->type == INST_LOAD_STORE;
}

uint64_t instruction_get_branch_target(const Instruction* inst, uint64_t pc) {
    if (!inst || inst->type != INST_BRANCH || inst->operand_count < 1) {
        return 0;
    }
    
    const Operand* target = &inst->operands[0];
    switch (target->type) {
        case OP_IMMEDIATE:
            return pc + target->value.immediate;
        case OP_REGISTER:
            return 0;
        default:
            return 0;
    }
}

bool instruction_modifies_register(const Instruction* inst, uint8_t reg) {
    if (!inst) return false;
    if (inst->dest_reg == reg) return true;
    
    switch (inst->type) {
        case INST_LOAD_STORE:
            if (inst->operand_count >= 1 && 
                inst->operands[0].type == OP_MEMORY &&
                inst->operands[0].value.mem.base_reg == reg) {
                return true;
            }
            break;
            
        case INST_BRANCH:
            if (reg == 30 && inst->opcode == 0x25) {
                return true;
            }
            break;
            
        default:
            break;
    }
    
    return false;
}

bool instruction_reads_register(const Instruction* inst, uint8_t reg) {
    if (!inst) return false;
    
    for (uint8_t i = 0; i < inst->operand_count; i++) {
        const Operand* op = &inst->operands[i];
        switch (op->type) {
            case OP_REGISTER:
                if (op->value.reg == reg) return true;
                break;
                
            case OP_MEMORY:
                if (op->value.mem.base_reg == reg ||
                    op->value.mem.index_reg == reg) {
                    return true;
                }
                break;
                
            default:
                break;
        }
    }
    
    return false;
}

bool instruction_can_be_reordered(const Instruction* inst) {
    if (!inst) return false;
    
    switch (inst->type) {
        case INST_BRANCH:
        case INST_SYSTEM:
        case INST_LOAD_STORE:
            return false;
        default:
            return true;
    }
} 