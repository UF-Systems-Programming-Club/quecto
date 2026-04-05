#include <stdio.h>

#include "symbol_table.h"
#include "common.h"

const char *symbol_type_to_string_table[] = {
    [SYM_TYPE_VARIABLE] = "variable",
    [SYM_TYPE_PROCEDURE] = "procedure"
};

const uint32_t quecto_primitive_width[] = {
    [QUECTO_COMP_INT] = -1, // depends on containter or other known types in an expression
    [QUECTO_I8] = 1,
    [QUECTO_U8] = 1,
    [QUECTO_I32] = 4,
    [QUECTO_U32] = 4,
    [QUECTO_ARRAY] = -1,
    [QUECTO_UNKNOWN] = -1,
};

QuectoType default_integer_type = (QuectoType) { .type = QUECTO_COMP_INT };

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
        if (symbol_table->table.keys[i].data == NULL) continue;

        SymbolData *data = (SymbolData*) symbol_table->table.items[i];

        print_indent(1, "\"%.*s\":\t{ type:\t%s,\tstack offset:\t%d },\n",
            (int)symbol_table->table.keys[i].size,
            (char*)symbol_table->table.keys[i].data,
            symbol_type_to_string_table[data->type],
            data->stack_offset);
    }
}


bool quecto_types_equal(QuectoType *a, QuectoType *b) {
    if (a == b) return true; // for interned values
    
    if (a && b && a->type == b->type || a && b && quecto_is_integer(a) && b->type == QUECTO_COMP_INT) {
        if (a->inner != NULL && b->inner != NULL) {
            return quecto_types_equal(a->inner, b->inner);
        }
        return true;
    }
    return false;
}


bool quecto_is_integer(QuectoType *a) {
    switch (a->type) {
        case QUECTO_I32:
        case QUECTO_U32:
        case QUECTO_I8:
        case QUECTO_U8:
            return true;
        default:
            return false;
    }
}


void print_type(QuectoType *qtype) {
    if (qtype == NULL) {
        printf("(null)");
        return;
    }
    switch(qtype->type) {
        case QUECTO_UNKNOWN:     printf("unknown"); break;
        case QUECTO_I32:         printf("i32"); break;
        case QUECTO_U32:         printf("u32"); break;
        case QUECTO_I8:          printf("i8"); break;
        case QUECTO_U8:          printf("u8"); break;
        case QUECTO_COMP_INT:    printf("comptime_int"); break;
        case QUECTO_ARRAY:
            print_type(qtype->inner);
            printf("[%lu]", qtype->array_size);
            break;
        default:
            UNREACHABLE("Non-existent Quecto Type");
            break;
    }
}
