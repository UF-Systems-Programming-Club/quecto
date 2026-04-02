#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
  Arena *arena;
  HashTable *type_intern_table;
} AnalysisContext;

void analyze_ast(AnalysisContext *context, AST *ast);
void analyze_block(AnalysisContext *context, AST *block, SymbolTable *scope);
void analyze_procedure(AnalysisContext *context, AST *procedure);
void analyze_statement(AnalysisContext *context, AST *statement, SymbolTable *scope);

#endif
