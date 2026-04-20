#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
  Arena *arena;
  HashTable *type_intern_table;
  
  QuectoType **returns;
  size_t return_count;
  
  SymbolTable *symbol_table;
} AnalysisContext;

QuectoType *resolve_binary_type(QuectoType *left, QuectoType *right);
SymbolData *lookup_or_error(AnalysisContext *context, AST *symbol, const char *message);

void analyze_ast(AnalysisContext *context, AST *ast);
void analyze_block(AnalysisContext *context, AST *block);
void analyze_procedure(AnalysisContext *context, AST *procedure);
void analyze_statement(AnalysisContext *context, AST *statement);
void analyze_assignment(AnalysisContext *context, AST *assignment);
void analyze_declaration(AnalysisContext *context, AST *decl);
QuectoType *analyze_expression(AnalysisContext *context, AST *expr, QuectoType *expected);
QuectoType *analyze_call(AnalysisContext *context, AST *call);
QuectoType *analyze_index(AnalysisContext *context, AST *index);
QuectoType *analyze_binary_op(AnalysisContext *context, AST *op, QuectoType *expected);
QuectoType *analyze_list(AnalysisContext *context, AST *list, QuectoType *expected);
QuectoType *analyze_symbol(AnalysisContext *context, AST *symbol);

#endif
