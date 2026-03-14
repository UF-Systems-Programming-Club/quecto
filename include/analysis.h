#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "ast.h"
#include "symbol_table.h"

void analyze_ast(AST *ast);
void analyze_block(AST *block, SymbolTable *scope);
void analyze_procedure(AST *procedure);
void analyze_statement(AST *statement, SymbolTable *scope);

#endif
