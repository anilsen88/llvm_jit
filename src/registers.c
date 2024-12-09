#include "registers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* error_strings[] = {
    "Success",
    "Null parameter",
    "Invalid register number",
    "Attempted to use SP (X31) as general purpose register",
    "Invalid vector register"
};

const char* registers_get_error_string(RegisterResult result) {
    if (result >= 0) return error_strings[0];
    size_t index = -result;
    if (index >= sizeof(error_strings) / sizeof(error_strings[0])) {
        return "Unknown error";
    }
    return error_strings[index];
}

RegisterFile* registers_create(void) {
    RegisterFile* regs = (RegisterFile*)calloc(1, sizeof(RegisterFile));
    if (regs) {
        registers_reset(regs);
    }
    return regs;
}

void registers_destroy(RegisterFile* regs) {
    free(regs);
}

void registers_reset(RegisterFile* regs) {
    if (!regs) return;
    memset(regs->x, 0, sizeof(regs->x));
    memset(regs->v, 0, sizeof(regs->v));
    regs->fpsr = 0;
    regs->fpcr = 0;
}

RegisterResult registers_get_x(const RegisterFile* regs, uint8_t reg, uint64_t* value) {
    if (!regs || !value) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_REGS) return REG_ERROR_INVALID_REG;
    if (reg == ARM64_REG_X31) return REG_ERROR_SP_AS_GPR;
    *value = regs->x[reg];
    return REG_SUCCESS;
}

RegisterResult registers_set_x(RegisterFile* regs, uint8_t reg, uint64_t value) {
    if (!regs) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_REGS) return REG_ERROR_INVALID_REG;
    if (reg == ARM64_REG_X31) return REG_ERROR_SP_AS_GPR;
    regs->x[reg] = value;
    return REG_SUCCESS;
}

RegisterResult registers_get_w(const RegisterFile* regs, uint8_t reg, uint32_t* value) {
    if (!regs || !value) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_REGS) return REG_ERROR_INVALID_REG;
    if (reg == ARM64_REG_X31) return REG_ERROR_SP_AS_GPR;
    *value = (uint32_t)regs->x[reg];
    return REG_SUCCESS;
}

RegisterResult registers_set_w(RegisterFile* regs, uint8_t reg, uint32_t value) {
    if (!regs) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_REGS) return REG_ERROR_INVALID_REG;
    if (reg == ARM64_REG_X31) return REG_ERROR_SP_AS_GPR;
    regs->x[reg] = (regs->x[reg] & 0xFFFFFFFF00000000ULL) | value;
    return REG_SUCCESS;
}

RegisterResult registers_get_vector(const RegisterFile* regs, uint8_t reg, uint8_t* data) {
    if (!regs || !data) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_VECTOR_REGS) return REG_ERROR_INVALID_VECTOR;
    memcpy(data, regs->v[reg], 16);
    return REG_SUCCESS;
}

RegisterResult registers_set_vector(RegisterFile* regs, uint8_t reg, const uint8_t* data) {
    if (!regs || !data) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_VECTOR_REGS) return REG_ERROR_INVALID_VECTOR;
    memcpy(regs->v[reg], data, 16);
    return REG_SUCCESS;
}

RegisterResult registers_get_vector_lane(const RegisterFile* regs, uint8_t reg, uint8_t lane, uint64_t* value) {
    if (!regs || !value) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_VECTOR_REGS) return REG_ERROR_INVALID_VECTOR;
    if (lane > 1) return REG_ERROR_INVALID_REG;
    memcpy(value, &regs->v[reg][lane * 8], 8);
    return REG_SUCCESS;
}

RegisterResult registers_set_vector_lane(RegisterFile* regs, uint8_t reg, uint8_t lane, uint64_t value) {
    if (!regs) return REG_ERROR_NULL_PARAM;
    if (reg >= ARM64_NUM_VECTOR_REGS) return REG_ERROR_INVALID_VECTOR;
    if (lane > 1) return REG_ERROR_INVALID_REG;
    memcpy(&regs->v[reg][lane * 8], &value, 8);
    return REG_SUCCESS;
}

bool registers_get_flag_n(const RegisterFile* regs) {
    return regs && ((regs->x[ARM64_REG_NZCV] >> 31) & 1);
}

bool registers_get_flag_z(const RegisterFile* regs) {
    return regs && ((regs->x[ARM64_REG_NZCV] >> 30) & 1);
}

bool registers_get_flag_c(const RegisterFile* regs) {
    return regs && ((regs->x[ARM64_REG_NZCV] >> 29) & 1);
}

bool registers_get_flag_v(const RegisterFile* regs) {
    return regs && ((regs->x[ARM64_REG_NZCV] >> 28) & 1);
}

void registers_set_flags(RegisterFile* regs, bool n, bool z, bool c, bool v) {
    if (!regs) return;
    uint32_t nzcv = 0;
    nzcv |= (n ? 1U : 0U) << 31;
    nzcv |= (z ? 1U : 0U) << 30;
    nzcv |= (c ? 1U : 0U) << 29;
    nzcv |= (v ? 1U : 0U) << 28;
    regs->x[ARM64_REG_NZCV] = nzcv;
}

RegisterResult registers_get_pc(const RegisterFile* regs, uint64_t* value) {
    if (!regs || !value) return REG_ERROR_NULL_PARAM;
    *value = regs->x[ARM64_REG_PC];
    return REG_SUCCESS;
}

RegisterResult registers_set_pc(RegisterFile* regs, uint64_t value) {
    if (!regs) return REG_ERROR_NULL_PARAM;
    regs->x[ARM64_REG_PC] = value;
    return REG_SUCCESS;
}

RegisterResult registers_get_sp(const RegisterFile* regs, uint64_t* value) {
    if (!regs || !value) return REG_ERROR_NULL_PARAM;
    *value = regs->x[ARM64_REG_SP];
    return REG_SUCCESS;
}

RegisterResult registers_set_sp(RegisterFile* regs, uint64_t value) {
    if (!regs) return REG_ERROR_NULL_PARAM;
    regs->x[ARM64_REG_SP] = value;
    return REG_SUCCESS;
}

RegisterResult registers_save_state(const RegisterFile* regs, void* buffer) {
    if (!regs || !buffer) return REG_ERROR_NULL_PARAM;
    memcpy(buffer, regs, sizeof(RegisterFile));
    return REG_SUCCESS;
}

RegisterResult registers_load_state(RegisterFile* regs, const void* buffer) {
    if (!regs || !buffer) return REG_ERROR_NULL_PARAM;
    memcpy(regs, buffer, sizeof(RegisterFile));
    return REG_SUCCESS;
}

void registers_print_state(const RegisterFile* regs) {
    if (!regs) {
        printf("Error: NULL register file\n");
        return;
    }
    
    printf("Register State:\n");
    for (int i = 0; i < 31; i++) {
        printf("X%-2d: 0x%016lx  W%-2d: 0x%08x", 
               i, regs->x[i], 
               i, (uint32_t)regs->x[i]);
        if (i % 2 == 1) printf("\n");
        else printf("    ");
    }
    
    uint64_t sp, pc;
    registers_get_sp(regs, &sp);
    registers_get_pc(regs, &pc);
    printf("\nSP:  0x%016lx  WSP: 0x%08x\n", sp, (uint32_t)sp);
    printf("PC:  0x%016lx\n", pc);
    
    printf("NZCV: [N=%d Z=%d C=%d V=%d]\n",
           registers_get_flag_n(regs), registers_get_flag_z(regs),
           registers_get_flag_c(regs), registers_get_flag_v(regs));
    printf("FPSR: 0x%08x  FPCR: 0x%08x\n", regs->fpsr, regs->fpcr);
    
    printf("\nVector Registers (preview):\n");
    for (int i = 0; i < 4; i++) {
        printf("V%-2d: ", i);
        for (int j = 15; j >= 0; j--) {
            printf("%02x", regs->v[i][j]);
            if (j % 8 == 0) printf(" ");
        }
        printf("\n");
    }
    printf("... (use registers_get_vector for full vector state)\n");
} 