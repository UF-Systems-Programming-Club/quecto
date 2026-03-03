#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "ast.h"

typedef struct {
    TokenArray tokens;
    int current;
    bool error;
    SymbolTable *cur_symbol_table;
    Arena *arena;
} ParserState;

AST *parse_program(ParserState *parser);
AST *parse_expression(ParserState *parser, int min_prec);
int print_parser_error(ParserState *parser, const char *format, ...);

#endif
