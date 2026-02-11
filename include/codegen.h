#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "AST.h"

typedef enum {
    BIT_8,
    BIT_16,
    BIT_32,
    BIT_64,
} RegisterSizes;

typedef enum {
    LOC_REGISTER,
    LOC_STACK,
    LOC_IMMEDIATE
} LocType;

typedef struct {
    LocType type;
    union {
        int stack_offset; // negative offset from rbp
        int register_index;
        unsigned int value;
    };
} Loc;

typedef int Symbol;

typedef struct {
    Loc locs[512];
} LocTable;

extern int current_symbol;
extern LocTable loc_table;

extern const char *register_list[][4];
extern bool register_free_list[];

int allocate_register();
void free_register(int reg);
Symbol generate_ast_assembly(FILE *file, AST *ast);

#endif
