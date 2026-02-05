#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdbool.h>
#include "AST.h"

typedef enum {
    BITS_8,
    BITS_16,
    BITS_32,
    BITS_64,
} RegisterSizes;

typedef enum {
    LOC_REGISTER,
    LOC_STACK
} LocType;

typedef struct {
    LocType type;
    union {
        int stack_offset; // positive offset from rsp
        int register_index;
    };
} Loc;

extern const char *register_list[][4];
extern bool register_free_list[];

int allocate_register();
void free_register(int reg);
void generate_load_register_with_int(FILE *file, int reg, int val);
void generate_add_registers(FILE *file, int reg1, int reg2);
Loc generate_ast_assembly(FILE *file, AST *ast);

#endif