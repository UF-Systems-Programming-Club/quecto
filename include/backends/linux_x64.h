#ifndef LINUX_X64_H
#define LINUX_X64_H

typedef enum {
    X64_ADD,
    X64_SUB,
    X64_IMUL,
    X64_DIV,
    X64_CMP,
    X64_MOV,
    X64_RET,
    X64_CALL,
} MachOpcode;

typedef enum {
    MACH_VREG,
    MACH_REG,
    MACH_STACK,
    MACH_IMM,
} MachOperandType;

typedef struct {
    MachOperandType type;
    union {
        int vreg;
        int reg;
        int imm;
        int stack_offset;
    };
} MachOperand;

typedef struct {
    MachOpcode opcode;
    MachOperand dest;
    MachOperand arg1;
    MachOperand arg2;
} MachOp;

typedef struct {
    MachOp *items;
    size_t count;
    size_t capacity;
} MachCode;

#endif
