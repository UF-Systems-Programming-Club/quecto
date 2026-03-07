#include <stdio.h>

#include "parser.h"
#include "ast.h"
#include "common.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "error.h"

int get_token_precedence_table[] = {
    [TOKEN_EQUALS_EQUALS] = 1,
    [TOKEN_GREATER_EQUALS] = 1,
    [TOKEN_LESS_EQUALS] = 1,
    [TOKEN_LESS_THAN] = 1,
    [TOKEN_GREATER_THAN] = 1,
    [TOKEN_PLUS] = 2,
    [TOKEN_MINUS] = 2,
    [TOKEN_MULTIPLY] = 3,
    [TOKEN_DIVIDE] = 3,
};

bool token_is_operator(TokenType type) {
    switch (type) {
        case TOKEN_EQUALS_EQUALS:
        case TOKEN_GREATER_EQUALS:
        case TOKEN_LESS_EQUALS:
        case TOKEN_LESS_THAN:
        case TOKEN_GREATER_THAN:
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
            return true;
        default:
            return false;
    }
}

int get_token_type_precedence(TokenType type) {
    if (token_is_operator(type)) return get_token_precedence_table[type];
    return -1;
}

Token *get_next_token(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return &parser->tokens.items[parser->tokens.count - 1];
    return &parser->tokens.items[parser->current++];
}

TokenType peek_next_token_type(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return parser->tokens.items[parser->tokens.count - 1].type;

    return parser->tokens.items[parser->current].type;
}

bool match_next_token(ParserState *parser, Token *tok, TokenType type) {
    if (peek_next_token_type(parser) == type) {
        *tok = *get_next_token(parser);
        return true;
    }
    return false;
}

bool expect_next_token(ParserState *parser, Token *tok, TokenType type) {
    if (match_next_token(parser, tok, type)) {
        return true;
    } else {
        report_error(tok->row, tok->col, "expected \"%s\" but got \"%s\" instead",
                     token_to_string_table[type],
                     token_to_string_table[peek_next_token_type(parser)]);
        return false;
    }
}

bool expect_next_token_multiple(ParserState *parser, Token *tok, int count, ...) {
    assert(count > 1 && "Just use expect_next_token if you're only matching against one token type");
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        TokenType type = va_arg(args, TokenType);
        if (match_next_token(parser, tok, type)) {
            return true;
        }
    }
    va_end(args);
    va_start(args, count);

    report_error(tok->row, tok->col, "");
    for (int i = 0; i < count - 1; i++) {
        printf("\"%s\", ", token_to_string_table[va_arg(args, TokenType)]);
    }
    printf("or \"%s\", ", token_to_string_table[va_arg(args, TokenType)]);
    printf("but got \"%s\" instead\n", token_to_string_table[peek_next_token_type(parser)]);

    va_end(args);
    return false;
}

AST *parse_expression(ParserState *parser, int min_prec) {
    Token tok;
    if (!expect_next_token_multiple(parser, &tok, 4, TOKEN_INT_LIT, TOKEN_FLOAT_LIT, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN))
        return NULL;

    AST *left = arena_alloc_type(parser->arena, AST);

    switch (tok.type) {
        case TOKEN_INT_LIT:
            left->type = AST_INT_LIT;
            left->int_lit = tok.int_lit;
            break;
        case TOKEN_FLOAT_LIT:
            left->type = AST_FLOAT_LIT;
            left->float_lit = tok.float_lit;
            break;
        case TOKEN_IDENTIFIER:
            if (get_symbol(parser->cur_symbol_table, tok.identifier) == NULL) {
                report_error(tok.row, tok.col, "undeclared identifier");
                return NULL;
            }
            left->type = AST_SYMBOL;
            left->symbol_table = parser->cur_symbol_table;
            left->ident = tok.identifier;
            break;
        case TOKEN_OPEN_PAREN:
            left = parse_expression(parser, 0);
            if (left == NULL) return NULL;

            if (!expect_next_token(parser, &tok, TOKEN_CLOSE_PAREN)) return NULL;
            break;
    }

    // NOTE: utilizes the fact that get precedence returns -1 when the next token is
    // not an operator
    while (get_token_type_precedence(peek_next_token_type(parser)) > min_prec) {
        AST *op = arena_alloc_type(parser->arena, AST);
        op->type = AST_BINARY_OP;
        op->left = left;
        tok = *get_next_token(parser);
        switch (tok.type) {
            case TOKEN_EQUALS_EQUALS:   op->op = OP_EQUALS; break;
            case TOKEN_LESS_THAN:       op->op = OP_LESS_THAN; break;
            case TOKEN_GREATER_THAN:    op->op = OP_GREATER_THAN; break;
            case TOKEN_LESS_EQUALS:     op->op = OP_LESS_EQUALS; break;
            case TOKEN_GREATER_EQUALS:  op->op = OP_GREATER_EQUALS; break;
            case TOKEN_PLUS:            op->op = OP_PLUS;     break;
            case TOKEN_MINUS:           op->op = OP_MINUS;    break;
            case TOKEN_MULTIPLY:        op->op = OP_MULTIPLY; break;
            case TOKEN_DIVIDE:          op->op = OP_DIVIDE;   break;
        }

        op->right = parse_expression(parser, get_token_type_precedence(tok.type));
        if (op->right == NULL) return NULL;

        left = op;
    }

    return left;
}

AST *parse_statement(ParserState *parser) {
    Token tok;

    AST *statement = arena_alloc_type(parser->arena, AST);

    if (match_next_token(parser, &tok, TOKEN_OPEN_CURLY)) {
        statement->type = AST_BLOCK;

        // create a symbol table for new scope
        SymbolTable *symbol_table = calloc(1, sizeof(SymbolTable));
        symbol_table->prev = parser->cur_symbol_table;
        parser->cur_symbol_table = symbol_table;

        while (!match_next_token(parser, &tok, TOKEN_CLOSE_CURLY)) {
            AST *s = parse_statement(parser);
            array_append(*statement, s);
        }

        parser->cur_symbol_table = symbol_table->prev;

        //if (!expect_next_token(parser, &tok, TOKEN_CLOSE_CURLY)) return NULL;
        return statement;
    } else if (match_next_token(parser, &tok, TOKEN_IDENTIFIER)) {
        Token ident = tok;
        AST *symbol = arena_alloc_type(parser->arena, AST);
        symbol->type = AST_SYMBOL;
        symbol->ident = ident.identifier;
        symbol->symbol_table = parser->cur_symbol_table;
        statement->symbol = symbol;

        if (match_next_token(parser, &tok, TOKEN_COLON)) {
            if (match_next_token(parser, &tok, TOKEN_IDENTIFIER)) { printf("types not implemented yet\n"); assert(0); }
            if (!expect_next_token(parser, &tok, TOKEN_EQUALS)) return NULL;

            statement->type = AST_DECL;
            insert_symbol(parser->arena, parser->cur_symbol_table, symbol->ident, SYM_TYPE_VARIABLE);
            statement->expr = parse_expression(parser, 0);
        } else if (match_next_token(parser, &tok, TOKEN_EQUALS)) {
            statement->type = AST_ASSIGNMENT;
            statement->expr = parse_expression(parser, 0);
        } else {
            statement = parse_expression(parser, 0);
        }
    } else if (match_next_token(parser, &tok, TOKEN_RETURN)) {
        statement->type = AST_RETURN;
        statement->expr = parse_expression(parser, 0);
    } else if (match_next_token(parser, &tok, TOKEN_IF)) {
        statement->type = AST_IF;
        statement->condition = parse_expression(parser, 0);
        statement->block = parse_statement(parser);
        return statement;
    } else {
        statement = parse_expression(parser, 0);
    }

    if (!match_next_token(parser, &tok, TOKEN_SEMICOLON)) return NULL;

    return statement;
}

AST *parse_program(ParserState *parser) {
    AST *program = arena_alloc_type(parser->arena, AST);
    program->type = AST_PROGRAM;

    // TODO: implement error synchronization
    while (peek_next_token_type(parser) != TOKEN_EOF) {
        AST *statement = parse_statement(parser);
        if (statement == NULL) return program;
        array_append(*program, statement);
    }
    // NOTE: probably doesn't need to be done but consumes EOF for sake of completion
    get_next_token(parser);

    return program;
}
