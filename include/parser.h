#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "ast.h"

typedef enum {
    PCLF_DECL,
    PCLF_ASSIGN,
    PCLF_BLOCK,
    PCLF_WHILE,
    PCLF_IF,
    PCLF_RET,
    PCLF_EXPR,
} ParserClsfcn; 

typedef struct {
    TokenArray tokens;
    int current;
    HashTable *type_intern_table;
    Arena *arena;
} ParserState;
// TODO: decl will need to be changed to var_decl in the future
// because there will also be type decl and const decl i think

AST *parse_program(ParserState *parser); 
AST *parse_extern(ParserState *parser); 
AST *parse_procedure(ParserState *parser);
AST *parse_param_declaration(ParserState *parser);
size_t parse_params(ParserState *parser, AST *list[MAX_PARAMS]);
size_t parse_returns(ParserState *parser, AST *list[MAX_PARAMS]);

QuectoType *parse_type(ParserState *parser);

AST *parse_statement(ParserState *parser);
AST *parse_block(ParserState *parser); 
AST *parse_if(ParserState *parser); 
AST *parse_if_chain(ParserState *parser); 
AST *parse_while(ParserState *parser); 
AST *parse_assignment(ParserState *parser);
AST *parse_braced_initializer(ParserState *parser);
AST *parse_return(ParserState *parser); 
AST *parse_stmt_declaration(ParserState *parser);

AST *parse_expression(ParserState *parser, int min_prec);

// TODO: change this out to separate param and return list
size_t parse_args(ParserState *parser, AST *list[MAX_PARAMS]);

#endif
