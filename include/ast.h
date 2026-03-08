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
    AST_IF,
    AST_ELIF,
    AST_ELSE,
    AST_WHILE,
    AST_SYMBOL,
    AST_INT_LIT,
    AST_FLOAT_LIT,
} ASTType;

typedef enum {
    OP_EQUALS,
    OP_LESS_EQUALS,
    OP_GREATER_EQUALS,
    OP_LESS_THAN,
    OP_GREATER_THAN,
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,
} BinaryOp;

typedef struct AST {
    ASTType type;
    union {
        // Program and block
        struct {
            struct AST **items;
            size_t count;
            size_t capacity;
        };

        // Binary op for expressions
        struct {
            BinaryOp op;
            struct AST *left;
            struct AST *right;
        };

        // Declaration and Assignment
        struct {
            struct AST *symbol;
            struct AST *expr;
        };

        // If, Elif, Else, While
        struct {
            struct AST *condition; // NULL for else statements
            struct AST *then;
            struct AST *otherwise; // else (cant name it else b/c keyword and cant think of a better word)
        };

        // Symbol
        struct {
            SymbolTable *symbol_table;
            const char *ident; // NOTE: this is a pointer to the tokenized identifier
                               // so every symbol has a unique string but not the
                               // authority to do anything to it (which makes sense)
        };

        // number literals in expressions
        unsigned int int_lit;
        float float_lit;
    };
} AST;

void print_ast(AST* ast, int indent);

#endif
