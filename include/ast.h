#ifndef AST_H
#define AST_H

#include "common.h"
#include "tokenizer.h"
#include "symbol_table.h"

typedef enum {
    AST_PROGRAM,
    AST_BINARY_OP,
    AST_PROCEDURE,
    AST_EXTERN,
    AST_CALL,
    AST_INDEX,
    AST_LIST,
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
    OP_NEQUALS,
    OP_LESS_EQUALS,
    OP_GREATER_EQUALS,
    OP_LESS_THAN,
    OP_GREATER_THAN,
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_COUNT,
} BinaryOp;

typedef struct AST {
    ASTType type;
    size_t line, col; // for starting point of expr and for debugging
    QuectoType *evaled_type;
    union {
        // Program and block and list
        struct {
            struct AST **items;
            size_t count;
            size_t capacity;
        };

        // Extern
        struct {
            struct AST *externed;
        };

        // Procedure
        struct {
            struct AST *name;
            struct AST *params[MAX_PARAMS]; // probably better to go with hardcode MAX_PARAMS could do variable length but I dont think its important
            struct AST *returns[MAX_PARAMS];
            size_t param_count;
            size_t return_count;
            struct AST *body;

            SymbolTable *symbols;
        };

        // Binary op for expressions
        struct {
            BinaryOp op;
            struct AST *left;
            struct AST *right;
        };

        // Calls
        struct {
            struct AST *callee;
            struct AST *args[MAX_PARAMS];
            size_t arg_count;
        };

        // Indexing
        struct {
            struct AST *access; // symbol
            struct AST *index; // expr
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
            const char *ident; // NOTE: this is a pointer to the tokenized identifier
                               // so every symbol has a unique string but not the
                               // authority to do anything to it (which makes sense)
        };

        // number literals in expressions
        unsigned int int_lit;
        float float_lit;
    };
} AST;

void print_ast(AST *ast, int indent);
bool op_is_conditional(BinaryOp op);
bool op_is_arithmetic(BinaryOp op);
BinaryOp op_opposite(BinaryOp op);

#endif
