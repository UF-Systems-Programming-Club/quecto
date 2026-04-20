#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "common.h"


typedef enum {
    QUECTO_UNKNOWN, // used if type is inferred / program is invalid if something is this after analysis
    QUECTO_COMP_INT,
    QUECTO_U8,
    QUECTO_I8,
    QUECTO_U32,
    QUECTO_I32,
    QUECTO_ARRAY,
    QUECTO_COUNT
} QuectoPrimitiveType;

typedef struct QuectoType {
    QuectoPrimitiveType type;
    union {
        struct {
            struct QuectoType *inner;
            size_t array_size;
        };   
    };
} QuectoType;

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
            bool externed;
            QuectoType *param_types[MAX_PARAMS];
            QuectoType *return_types[MAX_PARAMS];
            int local_var_size;
        };
        struct {
            QuectoType *qtype;
            int stack_offset; // from bp
        };
    };
} SymbolData;

typedef struct SymbolTable {
    struct SymbolTable *prev;
    HashTable table;
} SymbolTable;

extern const char *symbol_type_to_string_table[];
extern const uint32_t quecto_primitive_width[QUECTO_COUNT];
extern QuectoType default_integer_type; // default integer type

void print_symbol_table(SymbolTable *symbol_table, int indent);
SymbolData *insert_symbol(Arena *arena, SymbolTable *symbol_table, const char *symbol, SymbolType type);
void *get_symbol(SymbolTable *symbol_table, const char *str);

bool quecto_types_equal(QuectoType *a, QuectoType *b);
bool quecto_is_primitive(QuectoType *a);
bool quecto_is_integer(QuectoType *a);
bool quecto_is_signed(QuectoType *a);
int quecto_type_size(QuectoType *a);
void print_type(QuectoType *type);
 
#endif
