#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"

typedef enum {
    SYM_TYPE_VARIABLE,
} SymbolType;

typedef struct {
    SymbolType type;
    int stack_offset;
} SymbolData;

typedef struct SymbolTable {
    struct SymbolTable *prev;
    HashTable table;
} SymbolTable;

void print_symbol_table(SymbolTable *symbol_table, int indent);
void insert_symbol(SymbolTable *symbol_table, const char *symbol, SymbolType type);
void *get_symbol(SymbolTable *symbol_table, const char *str);

#endif
