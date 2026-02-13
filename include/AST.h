#ifndef AST_H
#define AST_H

#include "tokenizer.h"

typedef enum {
    AST_BINARY_OP,
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
    OP_ASSIGN,
} BinaryOp;

typedef struct AST {
    ASTType type;
    union {
        struct {
            BinaryOp op;
            struct AST *left;
            struct AST *right;
        };
        struct {
            char *symbol;
            struct AST *expr;
        };
        struct {
                struct AST **items;
                size_t count;
                size_t capacity;
        } block;
        unsigned int int_lit;
        float float_lit;
    };
} AST;

AST evaluate_ast(AST *ast);
void print_ast(AST* ast);

#endif
