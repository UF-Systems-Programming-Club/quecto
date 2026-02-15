#ifndef IR_H
#define IR_H

#include "ast.h"

typedef enum {
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,
    INST_LOAD,
} InstType;

typedef struct {
    InstType type;
    int dest;
    int arg1;
    int arg2;
} Inst;

typedef struct {
    Inst *items;
    size_t count;
    size_t capacity;
} InstList;

typedef struct {
    int start;
    int end;
    // both inclusive
} LiveInterval;

typedef struct {
    LiveInterval *items;
    size_t count;
    size_t capacity;
} IntervalList;

extern int current_register;

int generate_expr_ir(InstList *ir, AST *expr);
void generate_ir_from_ast(InstList *ir, AST *ast);
void pretty_print_ir(InstList ir);

IntervalList create_live_intervals_from_ir(InstList ir);
void print_live_intervals(IntervalList intervals);

#endif
