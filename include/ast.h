#ifndef AST_H
#define AST_H

#include "common.h"
#include "tokenizer.h"
#include "symbol_table.h"

typedef enum {

    AST_BINARY_OP,
    AST_ASSIGNMENT,
    AST_BLOCK,
    AST_DECLARATION,
    AST_RETURN,
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
        // Binary op
        struct {
            BinaryOp op;
            struct AST *left;
            struct AST *right;
        };
        // Declaration and Assignment
        struct {
            char *symbol;
            struct AST *expr;
        };
        struct {
            SymbolTable* symbol_table;
            struct AST **items;
            size_t count;
            size_t capacity;
        } block;
        unsigned int int_lit;
        float float_lit;
    };

} AST;

void print_ast(AST* ast, int indent);

#endif
