#ifndef AST_H
#define AST_H

#include "common.h"
#include "tokenizer.h"
#include "symbol_table.h"

typedef enum {
    AST_PROGRAM,
    AST_BINARY_OP,
    AST_BLOCK,
    AST_DECL,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_SYMBOL,
    AST_INT_LIT,
    AST_FLOAT_LIT,
} ASTType;

typedef enum {
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,
} BinaryOp;

typedef struct AST {
    ASTType type;
    union {
        // TODO: should probably have anonymous structs inside the union? not sure, this would
        // be a pretty agressive renaming though, lots of changes
        struct {
            struct AST **items;
            size_t count;
            size_t capacity;
        } program, block;
        // Binary op
        struct {
            BinaryOp op;
            struct AST *left;
            struct AST *right;
        };
        // Declaration and Assignment
        struct {
            struct AST *symbol;
            struct AST *expr;
        } decl, assignment;
        struct {
            SymbolTable *symbol_table;
            char *ident;
        } symbol;
        unsigned int int_lit;
        float float_lit;
    };
} AST;

void print_ast(AST* ast, int indent);

#endif
