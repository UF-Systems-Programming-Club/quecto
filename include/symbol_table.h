#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"
#include <sys/_types/_null.h>
typedef enum {
    SYM_TYPE_VARIABLE,
} SymbolType;

typedef struct {
    SymbolType type;
} SymbolData;

typedef struct symbol_table_t {
    struct symbol_table_t *prev;
    HashTable table;
} SymbolTable;

void print_symbol_table(SymbolTable *symbol_table, int indent);

#endif
