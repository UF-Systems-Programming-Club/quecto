#include <stdio.h>

#include "symbol_table.h"
#include "common.h"

void print_symbol_table(SymbolTable *symbol_table, int indent) {
    for (int i = 0; i < symbol_table->table.capacity; i++) {
        if (symbol_table->table.keys[i] == NULL) continue;

        SymbolData *data = (SymbolData*) symbol_table->table.items[i];

        print_indent(1, "\"%s\": { type: %d },\n", symbol_table->table.keys[i], data->type);
    }
}
