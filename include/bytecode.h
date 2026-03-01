#ifndef BYTECODE_H
#define BYTECODE_H

#include "ast.h"

typedef enum {
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_LOAD, // TODO: currently load and store operate on the stack.
               // will need to seprate stack loads from regular loads
    INST_STORE,
    INST_MOV,
    INST_LOADI,
} OpCode;

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
    OpCode opcode;
    Operand dest;
    Operand arg1;
    Operand arg2;
} Bytecode;

typedef struct {
    Bytecode *items;
    size_t count;
    size_t capacity;
} BytecodeArray;

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
extern bool register_is_free[];

Operand generate_expr_ir(BytecodeArray *ir, AST *expr);
void generate_ir_from_ast(BytecodeArray *ir, AST *ast);
void pretty_print_ir(BytecodeArray ir);

IntervalArray create_live_intervals_from_ir(BytecodeArray ir);
void print_live_intervals(IntervalArray intervals);
LocationArray linear_scan_register_allocation(IntervalArray *intervals);

#endif
