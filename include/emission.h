#ifndef EMISSION_H
#define EMISSION_H

#include "ir.h"

typedef struct {
    Arena *arena;
    Procedure *procedure;
    HashTable slot_from_symbol;
    HashTable global_from_symbol;
    SymbolTable *scope;
    int current_block;
} EmitContext;

void emit_program(EmitContext *context, Program *into, AST *program);
void emit_procedure(EmitContext *context, Procedure *into, AST *procedure);
void emit_statement(EmitContext *context, AST *statement);
void emit_block(EmitContext *context, AST *block);
void emit_if(EmitContext *context, AST *_if);
void emit_if_chain(EmitContext *context, AST *_if, int end);
void emit_while(EmitContext *context, AST *_while);
void emit_return(EmitContext *context, AST *ret);
void emit_assign(EmitContext *context, AST *assign);
void emit_decl(EmitContext *context,  AST *decl);

Operand emit_expr(EmitContext *context, AST *expr);
Operand emit_call(EmitContext *context, AST *call, bool has_dest);
Operand emit_lhs(EmitContext *context, AST *lhs);

Operand emit_instr(EmitContext *context, Opcode opcode, Operand dest, Operand arg1, Operand arg2);
void emit_jmp(EmitContext *context, int target);
void emit_branch(EmitContext *context, int true_block, int false_block, AST *condition);
void emit_ret(EmitContext *context, Operand with);

#endif
