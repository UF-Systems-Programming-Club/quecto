#include <stdio.h>

#include "symbol_table.h"
#include "common.h"

void insert_symbol(SymbolTable *symbol_table, const char *symbol, SymbolType type) {
    HashTable *table = &symbol_table->table;

    SymbolData *data = calloc(1, sizeof(SymbolData));
    data->type = type;

    ht_insert(table, symbol, data);
}

void *get_symbol(SymbolTable *symbol_table, const char *str) {
    return ht_search(&symbol_table->table, str);
}

void print_symbol_table(SymbolTable *symbol_table, int indent) {
    for (int i = 0; i < symbol_table->table.capacity; i++) {
        if (symbol_table->table.keys[i] == NULL) continue;

        SymbolData *data = (SymbolData*) symbol_table->table.items[i];

        print_indent(1, "\"%s\": { type: %d },\n", symbol_table->table.keys[i], data->type);
    }
}
