#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum RegisterResult {
    REG_SUCCESS = 0,
    REG_ERROR_NULL_PARAM = -1,
    REG_ERROR_INVALID_REG = -2,
    REG_ERROR_SP_AS_GPR = -3,  
    REG_ERROR_INVALID_VECTOR = -4
} RegisterResult;

#define ARM64_REG_X0  0
#define ARM64_REG_X1  1
#define ARM64_REG_X2  2
#define ARM64_REG_X3  3
#define ARM64_REG_X4  4
#define ARM64_REG_X5  5
#define ARM64_REG_X6  6
#define ARM64_REG_X7  7
#define ARM64_REG_X8  8
#define ARM64_REG_X9  9
#define ARM64_REG_X10 10
#define ARM64_REG_X11 11
#define ARM64_REG_X12 12
#define ARM64_REG_X13 13
#define ARM64_REG_X14 14
#define ARM64_REG_X15 15
#define ARM64_REG_X16 16
#define ARM64_REG_X17 17
#define ARM64_REG_X18 18
#define ARM64_REG_X19 19
#define ARM64_REG_X20 20
#define ARM64_REG_X21 21
#define ARM64_REG_X22 22
#define ARM64_REG_X23 23
#define ARM64_REG_X24 24
#define ARM64_REG_X25 25
#define ARM64_REG_X26 26
#define ARM64_REG_X27 27
#define ARM64_REG_X28 28
#define ARM64_REG_X29 29  
#define ARM64_REG_X30 30  
#define ARM64_REG_X31 31  

#define ARM64_REG_W0  ARM64_REG_X0
#define ARM64_REG_W1  ARM64_REG_X1
#define ARM64_REG_W2  ARM64_REG_X2
#define ARM64_REG_W3  ARM64_REG_X3
#define ARM64_REG_W4  ARM64_REG_X4
#define ARM64_REG_W5  ARM64_REG_X5
#define ARM64_REG_W6  ARM64_REG_X6
#define ARM64_REG_W7  ARM64_REG_X7
#define ARM64_REG_W8  ARM64_REG_X8
#define ARM64_REG_W9  ARM64_REG_X9
#define ARM64_REG_W10 ARM64_REG_X10
#define ARM64_REG_W11 ARM64_REG_X11
#define ARM64_REG_W12 ARM64_REG_X12
#define ARM64_REG_W13 ARM64_REG_X13
#define ARM64_REG_W14 ARM64_REG_X14
#define ARM64_REG_W15 ARM64_REG_X15
#define ARM64_REG_W16 ARM64_REG_X16
#define ARM64_REG_W17 ARM64_REG_X17
#define ARM64_REG_W18 ARM64_REG_X18
#define ARM64_REG_W19 ARM64_REG_X19
#define ARM64_REG_W20 ARM64_REG_X20
#define ARM64_REG_W21 ARM64_REG_X21
#define ARM64_REG_W22 ARM64_REG_X22
#define ARM64_REG_W23 ARM64_REG_X23
#define ARM64_REG_W24 ARM64_REG_X24
#define ARM64_REG_W25 ARM64_REG_X25
#define ARM64_REG_W26 ARM64_REG_X26
#define ARM64_REG_W27 ARM64_REG_X27
#define ARM64_REG_W28 ARM64_REG_X28
#define ARM64_REG_W29 ARM64_REG_X29
#define ARM64_REG_W30 ARM64_REG_X30

#define ARM64_REG_SP   31  
#define ARM64_REG_WSP  31  
#define ARM64_REG_PC   32  
#define ARM64_REG_NZCV 33  

#define ARM64_NUM_REGS 34
#define ARM64_NUM_VECTOR_REGS 32

typedef struct RegisterFile {
    uint64_t x[ARM64_NUM_REGS];  
    uint8_t v[ARM64_NUM_VECTOR_REGS][16];
    uint32_t fpsr;  
    uint32_t fpcr; 
} RegisterFile;

RegisterFile* registers_create(void);
void registers_destroy(RegisterFile* regs);
void registers_reset(RegisterFile* regs);

/* 
 * For W registers:
 * Reading returns the lower 32 bits of the corresponding X register
 * Writing zero-extends the value and only modifies the lower 32 bits
 * The upper 32 bits of the X register are preserved on W register writes
 */

RegisterResult registers_get_x(const RegisterFile* regs, uint8_t reg, uint64_t* value);
RegisterResult registers_set_x(RegisterFile* regs, uint8_t reg, uint64_t value);
RegisterResult registers_get_w(const RegisterFile* regs, uint8_t reg, uint32_t* value);
RegisterResult registers_set_w(RegisterFile* regs, uint8_t reg, uint32_t value);

RegisterResult registers_get_vector(const RegisterFile* regs, uint8_t reg, uint8_t* data);
RegisterResult registers_set_vector(RegisterFile* regs, uint8_t reg, const uint8_t* data);
RegisterResult registers_get_vector_lane(const RegisterFile* regs, uint8_t reg, uint8_t lane, uint64_t* value);
RegisterResult registers_set_vector_lane(RegisterFile* regs, uint8_t reg, uint8_t lane, uint64_t value);

bool registers_get_flag_n(const RegisterFile* regs);
bool registers_get_flag_z(const RegisterFile* regs);
bool registers_get_flag_c(const RegisterFile* regs);
bool registers_get_flag_v(const RegisterFile* regs);
void registers_set_flags(RegisterFile* regs, bool n, bool z, bool c, bool v);

RegisterResult registers_get_pc(const RegisterFile* regs, uint64_t* value);
RegisterResult registers_set_pc(RegisterFile* regs, uint64_t value);
RegisterResult registers_get_sp(const RegisterFile* regs, uint64_t* value);
RegisterResult registers_set_sp(RegisterFile* regs, uint64_t value);

RegisterResult registers_save_state(const RegisterFile* regs, void* buffer);
RegisterResult registers_load_state(RegisterFile* regs, const void* buffer);
void registers_print_state(const RegisterFile* regs);

const char* registers_get_error_string(RegisterResult result);

#endif