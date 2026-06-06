#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
    int global_count;

    Arena *arena;
    HashTable *type_intern_table;

    ProcSignature *current_procedure;

    SymbolTable *symbol_table;
} AnalysisContext;

QuectoType *resolve_binary_type(QuectoType *left, QuectoType *right);
QuectoType *pointer_of(AnalysisContext *context, QuectoType *type);
SymbolData *lookup_or_error(AnalysisContext *context, AST *symbol, const char *message);

void analyze_ast(AnalysisContext *context, AST *ast);
void analyze_block(AnalysisContext *context, AST *block);
void analyze_procedure(AnalysisContext *context, AST *procedure, bool externed);
void analyze_statement(AnalysisContext *context, AST *statement);
void analyze_assignment(AnalysisContext *context, AST *assignment);
void analyze_declaration(AnalysisContext *context, AST *decl);
QuectoType *analyze_expression(AnalysisContext *context, AST *expr, QuectoType *expected);
QuectoType *analyze_call(AnalysisContext *context, AST *call);
QuectoType *analyze_index(AnalysisContext *context, AST *index);
QuectoType *analyze_ref(AnalysisContext *context, AST *ref);
QuectoType *analyze_binary_op(AnalysisContext *context, AST *op, QuectoType *expected);
QuectoType *analyze_list(AnalysisContext *context, AST *list, QuectoType *expected);
QuectoType *analyze_symbol(AnalysisContext *context, AST *symbol);
QuectoType *analyze_string_literal(AnalysisContext *context, AST *lit);

#endif
