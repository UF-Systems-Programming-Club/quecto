#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"

typedef enum {
    SYM_TYPE_VARIABLE,
    SYM_TYPE_PROCEDURE,
    SYM_TYPE_COUNT,
} SymbolType;

typedef struct {
    SymbolType type;
    union {
        struct {
            int param_count;
            int return_count;
            int local_var_size;
        };
        int stack_offset;
    };
} SymbolData;

typedef struct SymbolTable {
    struct SymbolTable *prev;
    HashTable table;
} SymbolTable;

extern const char *symbol_type_to_string_table[];

void print_symbol_table(SymbolTable *symbol_table, int indent);
SymbolData *insert_symbol(Arena *arena, SymbolTable *symbol_table, const char *symbol, SymbolType type);
void *get_symbol(SymbolTable *symbol_table, const char *str);

#endif
