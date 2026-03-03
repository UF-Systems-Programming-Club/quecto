#ifndef BYTECODE_H
#define BYTECODE_H

#include "ast.h"

typedef enum {
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_DIV,
    OPCODE_CMP_EQ,
    OPCODE_CMP_LT,
    OPCODE_CMP_GT,
    OPCODE_CMP_LEQ,
    OPCODE_CMP_GEQ,
    OPCODE_LOAD, // TODO: currently load and store operate on the stack.
               // will need to seperate stack loads from regular loads
    OPCODE_STORE,
    OPCODE_MOV,
    OPCODE_LOADI,
    OPCODE_RET,
} Opcode;

typedef enum {
    OPERAND_VREG,
    OPERAND_IMM,
    OPERAND_REG,
    OPERAND_STACK,
} OperandType;

typedef struct {
    OperandType type;
    union {
        int vreg;
        int reg;
        int imm;
        int stack_offset;
    };
} Operand;

typedef struct {
    Opcode opcode;
    Operand dest;
    Operand arg1;
    Operand arg2;
} Instr;

typedef struct {
    Instr *items;
    size_t count;
    size_t capacity;
} Bytecode;

typedef enum {
    LOC_REGISTER,
    LOC_STACK,
} LocationType;

typedef struct {
    LocationType type;
    union {
        int register_index;
        int stack_offset; // negative offset from rbp (so 8 means [bp - 8])
    };
} Location;

typedef struct {
    Location *items;
    size_t count;
    size_t capacity;
} LocationArray;

typedef struct {
    int vreg;
    int start; // inclusive
    int end;   // exclusive i.e. dies at end so can be merged with another interval
} Interval;

typedef struct {
    Interval *items;
    size_t count;
    size_t capacity;
} IntervalArray;

typedef struct {
} IntervalSet;

extern int vreg_count;
extern const char *registers[];
extern const char *registers_8bit_low[];
extern bool register_is_free[];

void gen_bytecode_from_ast(Bytecode *bytecode, AST *ast);
void pretty_print_bytecode(Bytecode bytecode);

IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode);
void print_live_intervals(IntervalArray intervals);
LocationArray linear_scan_register_allocation(IntervalArray *intervals);

#endif
