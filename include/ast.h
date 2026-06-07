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
    AST_ACCESS,
    AST_REF,
    AST_LIST,
    AST_BLOCK,
    AST_DECL,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_IF,
    AST_ELIF,
    AST_ELSE,
    AST_WHILE,
    AST_IDENTIFIER,
    AST_INT_LIT,
    AST_STR_LIT,
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
    Token token;
    QuectoType *resolved_qtype;
} ASTBase;

typedef ASTBase AST;
typedef DYN_ARR(AST *) ASTList;

#define CAST_AST(base, to, tag) (assert(base->type == tag), (to*)(base))

#define UNWRAP(...) __VA_ARGS__
#define DEFINE_AST(name, decls) typedef struct {ASTBase base; UNWRAP decls } AST##name;

DEFINE_AST(Program,    (ASTList decls;) )
DEFINE_AST(Extern,     (AST *decl;) )
DEFINE_AST(Proc,       (AST *name; ASTList params; ASTList rets; AST *body;) )
DEFINE_AST(Block,      (SymbolTable *symbols; ASTList stmts;) )
DEFINE_AST(ListConstruct,      (ASTList items;) )
DEFINE_AST(BinaryOp,   (BinaryOp op; AST *left; AST *right;) )
DEFINE_AST(Decl,       (AST *lhs; AST *rhs;) )
DEFINE_AST(Assign,     (AST *lhs; AST *rhs;) )
DEFINE_AST(Ref,        (AST *head;) )
DEFINE_AST(Index,      (AST *head; AST *index;) )
DEFINE_AST(Access,     (AST *head; AST *spec;) )
DEFINE_AST(Call,       (AST *ident; ASTList args;) )
DEFINE_AST(If,         (AST *cond; AST *then; AST *otherwise;) )
DEFINE_AST(While,      (AST *cond; AST *then; ))
DEFINE_AST(Return,     (AST *expr;) )
DEFINE_AST(Identifier, (const char *name;) )
DEFINE_AST(StringLit,  (const char *literal;) )
DEFINE_AST(IntLit,     (unsigned int literal;) )
DEFINE_AST(FloatLit,   (float literal;) )

#undef DEFINE_AST
#undef UNWRAP


AST *get_underlying_symbol_from(AST *index);
void print_ast(AST *ast, int indent);
bool op_is_conditional(BinaryOp op);
bool op_is_arithmetic(BinaryOp op);
BinaryOp op_opposite(BinaryOp op);

void ast_print(Arena *arena, AST *root);
String ast_str(Arena *arena, AST *root);
String astlist_str(Arena *arena, ASTList list, const char *delim, int indent);

#endif
