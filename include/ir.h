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

typedef enum {
    LOC_REGISTER,
    LOC_STACK,
} LocationType;

typedef struct {
    LocationType type;
    union {
        int register_index;
        int stack_offset; // negative offset from rbp (so 8 means [rbp - 8])
    };
} Location;

typedef struct {
    Location loc;
    int start;
    int end;
    // both inclusive
} Interval;

typedef struct {
    Interval *items;
    size_t count;
    size_t capacity;
} IntervalList;

extern int current_register;
extern const char *registers[];

int generate_expr_ir(InstList *ir, AST *expr);
void generate_ir_from_ast(InstList *ir, AST *ast);
void pretty_print_ir(InstList ir);

IntervalList create_live_intervals_from_ir(InstList ir);
void print_live_intervals(IntervalList intervals);
void linear_scan_register_allocation(IntervalList *intervals);

#endif
