#include "../codegen.h"

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
    X64_JMP,
    X64_JNE,
    X64_JE,
    X64_JL,
    X64_JG,
    X64_JLE,
    X64_JGE,
    X64_CALL,
    X64_SETE,
    X64_SETL,
    X64_SETG,
    X64_SETLE,
    X64_SETGE,
    X64_LEA,
    X64_PUSH,
    X64_POP,

    X64_LABEL = 50, // aimed to be removed
} X64_Opcode;

typedef enum {
    x64_EAX,
    x64_ECX,
    x64_EDX,
    x64_EDI,
    x64_R8D,
    x64_R9D,
    x64_R10D,
    x64_R11D,
    x64_ESI,

    x64_AL,
    x64_CL,
    x64_DL,
    x64_DIL,
    x64_R8B,
    x64_R9B,
    x64_R10B,
    x64_R11B,
    x64_SIL,

    x64_RAX,
    x64_RCX,
    x64_RDX,
    x64_RDI,
    x64_R8,
    x64_R9,
    x64_R10,
    x64_R11,
    x64_RSI,

    x64_RBP,
    x64_RSP,
} x64_Register;

int regsize_from_bytes(int bits);
x64_Register select_register(VregInfo info);
MachineOperand select_stack(CodegenInterface *iface, int slot);

extern CodegenBackend LINUX_X86_64_BACKEND;

#endif
