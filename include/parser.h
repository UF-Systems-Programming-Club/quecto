#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "ast.h"

typedef struct {
    TokenArray tokens;
    int current;
    SymbolTable *cur_symbol_table;
    Arena *arena;
} ParserState;

AST *parse_program(ParserState *parser);
AST *parse_procedure(ParserState *parser);
AST *parse_param_list(ParserState *parser);
AST *parse_return_list(ParserState *parser);
AST *parse_statement(ParserState *parser);
AST *parse_expression(ParserState *parser, int min_prec);
// TODO: decl will need to be changed to var_decl in the future
// because there will also be type decl and const decl i think
AST *parse_decl(ParserState *parser);
AST *parse_assignment(ParserState *parser);
AST *parse_if(ParserState *parser);
AST *parse_block(ParserState *parser);
AST *parse_return(ParserState *parser);

// TODO: change this out to separate param and return list
size_t parse_args(ParserState *parser, AST *list[MAX_PARAMS]);

#endif
