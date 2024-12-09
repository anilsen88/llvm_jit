#include "registers.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    memset(regs->x, 0, sizeof(regs->x));
    memset(regs->v, 0, sizeof(regs->v));
    regs->fpsr = 0;
    regs->fpcr = 0;
}

uint64_t registers_get_x(const RegisterFile* regs, uint8_t reg) {
    if (reg >= ARM64_NUM_REGS) return 0;
    return regs->x[reg];
}

void registers_set_x(RegisterFile* regs, uint8_t reg, uint64_t value) {
    if (reg >= ARM64_NUM_REGS) return;
    if (reg == ARM64_REG_X31) return;
    regs->x[reg] = value;
}

uint32_t registers_get_w(const RegisterFile* regs, uint8_t reg) {
    if (reg >= ARM64_NUM_REGS) return 0;
    return (uint32_t)regs->x[reg];
}

void registers_set_w(RegisterFile* regs, uint8_t reg, uint32_t value) {
    if (reg >= ARM64_NUM_REGS) return;
    if (reg == ARM64_REG_X31) return;
    regs->x[reg] = (regs->x[reg] & 0xFFFFFFFF00000000ULL) | value;
}

void registers_get_vector(const RegisterFile* regs, uint8_t reg, uint8_t* data) {
    if (reg >= ARM64_NUM_VECTOR_REGS || !data) return;
    memcpy(data, regs->v[reg], 16);
}

void registers_set_vector(RegisterFile* regs, uint8_t reg, const uint8_t* data) {
    if (reg >= ARM64_NUM_VECTOR_REGS || !data) return;
    memcpy(regs->v[reg], data, 16);
}

uint64_t registers_get_vector_lane(const RegisterFile* regs, uint8_t reg, uint8_t lane) {
    if (reg >= ARM64_NUM_VECTOR_REGS || lane > 1) return 0;
    uint64_t result;
    memcpy(&result, &regs->v[reg][lane * 8], 8);
    return result;
}

void registers_set_vector_lane(RegisterFile* regs, uint8_t reg, uint8_t lane, uint64_t value) {
    if (reg >= ARM64_NUM_VECTOR_REGS || lane > 1) return;
    memcpy(&regs->v[reg][lane * 8], &value, 8);
}

bool registers_get_flag_n(const RegisterFile* regs) {
    return (regs->x[ARM64_REG_NZCV] >> 31) & 1;
}

bool registers_get_flag_z(const RegisterFile* regs) {
    return (regs->x[ARM64_REG_NZCV] >> 30) & 1;
}

bool registers_get_flag_c(const RegisterFile* regs) {
    return (regs->x[ARM64_REG_NZCV] >> 29) & 1;
}

bool registers_get_flag_v(const RegisterFile* regs) {
    return (regs->x[ARM64_REG_NZCV] >> 28) & 1;
}

void registers_set_flags(RegisterFile* regs, bool n, bool z, bool c, bool v) {
    uint32_t nzcv = 0;
    nzcv |= (n ? 1U : 0U) << 31;
    nzcv |= (z ? 1U : 0U) << 30;
    nzcv |= (c ? 1U : 0U) << 29;
    nzcv |= (v ? 1U : 0U) << 28;
    regs->x[ARM64_REG_NZCV] = nzcv;
}

uint64_t registers_get_pc(const RegisterFile* regs) {
    return regs->x[ARM64_REG_PC];
}

void registers_set_pc(RegisterFile* regs, uint64_t value) {
    regs->x[ARM64_REG_PC] = value;
}

uint64_t registers_get_sp(const RegisterFile* regs) {
    return regs->x[ARM64_REG_SP];
}

void registers_set_sp(RegisterFile* regs, uint64_t value) {
    regs->x[ARM64_REG_SP] = value;
}

void registers_save_state(const RegisterFile* regs, void* buffer) {
    if (!buffer) return;
    memcpy(buffer, regs, sizeof(RegisterFile));
}

void registers_load_state(RegisterFile* regs, const void* buffer) {
    if (!buffer) return;
    memcpy(regs, buffer, sizeof(RegisterFile));
}

void registers_print_state(const RegisterFile* regs) {
    printf("Register State:\n");
    for (int i = 0; i < 31; i++) {
        printf("X%-2d: 0x%016lx", i, regs->x[i]);
        if (i % 2 == 1) printf("\n");
        else printf("  ");
    }
    printf("SP:  0x%016lx  PC:  0x%016lx\n", 
           registers_get_sp(regs), registers_get_pc(regs));
    printf("NZCV: [N=%d Z=%d C=%d V=%d]\n",
           registers_get_flag_n(regs), registers_get_flag_z(regs),
           registers_get_flag_c(regs), registers_get_flag_v(regs));
    printf("FPSR: 0x%08x  FPCR: 0x%08x\n", regs->fpsr, regs->fpcr);
} 