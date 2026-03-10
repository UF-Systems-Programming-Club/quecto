#include <stdio.h>

#include "symbol_table.h"
#include "common.h"

const char *symbol_type_to_string_table[] = {
    [SYM_TYPE_VARIABLE] = "variable",
    [SYM_TYPE_PROCEDURE] = "procedure"
};

static_assert(sizeof(symbol_type_to_string_table) / sizeof(char *) == SYM_TYPE_COUNT,
        "Every symbol type should have entry in string table.");

SymbolData *insert_symbol(Arena *arena, SymbolTable *symbol_table, const char *symbol, SymbolType type) {
    HashTable *table = &symbol_table->table;

    SymbolData *data = arena_alloc_type(arena, SymbolData);
    data->type = type;

    ht_insert(table, symbol, data);
    return data;
}

void *get_symbol(SymbolTable *symbol_table, const char *str) {
    SymbolTable *table = symbol_table;
    while (table != NULL) {
        void *result = ht_search(&table->table, str);
        if (result != NULL) return result;
        table = table->prev;
    }
    return NULL;
}

void print_symbol_table(SymbolTable *symbol_table, int indent) {
    for (int i = 0; i < symbol_table->table.capacity; i++) {
        if (symbol_table->table.keys[i] == NULL) continue;

        SymbolData *data = (SymbolData*) symbol_table->table.items[i];

        print_indent(1, "\"%s\":\t{ type:\t%s,\tstack offset:\t%d },\n",
            symbol_table->table.keys[i],
            symbol_type_to_string_table[data->type],
            data->stack_offset);
    }
}
