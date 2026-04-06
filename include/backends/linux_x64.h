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
    X64_CALL,

    X64_LABEL, // aimed to be removed
} X64_Opcode;

typedef enum {
    x64_EAX,
    x64_ECX,
    x64_EDX,
    x64_EDI,
    x64_ESI,

    x64_AL,
    x64_CL,
    x64_DL,
    x64_DIL,

    x64_RAX,
    x64_RCX,
    x64_RDX,
    x64_RDI
} x64_Register;

extern CodegenBackend LINUX_X86_64_BACKEND;

#endif
