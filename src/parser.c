#include <stdio.h>

#include "parser.h"
#include "ast.h"
#include "common.h"
#include "tokenizer.h"

int get_token_precedence_table[] = {
    [TOKEN_PLUS] = 1,
    [TOKEN_MINUS] = 1,
    [TOKEN_MULTIPLY] = 2,
    [TOKEN_DIVIDE] = 2,
};

bool token_is_operator(TokenType type) {
    return type == TOKEN_PLUS     ||
           type == TOKEN_MINUS    ||
           type == TOKEN_MULTIPLY ||
           type == TOKEN_DIVIDE;
}

int get_token_type_precedence(TokenType type) {
    if (token_is_operator(type)) return get_token_precedence_table[type];
    return -1;
}

Token *get_next_token(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return &parser->tokens.items[parser->tokens.count - 1];
    return &parser->tokens.items[parser->current++];
}

int print_parser_error(ParserState *parser, const char *format, ...) {
    va_list args;
    va_start(args, format);

    Token *tok = get_next_token(parser);
    printf("parser error at (line: %d, column: %d): ", tok->row, tok->col);
    int result = vprintf(format, args);

    va_end(args);
    return result;
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
        print_parser_error(parser,"expected \"%s\" but got \"%s\" instead\n",
            token_to_string_table[type],
            token_to_string_table[peek_next_token_type(parser)]);
        parser->error = true;
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

    parser->error = true;
    print_parser_error(parser, "");
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
    if (!expect_next_token_multiple(parser, &tok, 3, TOKEN_INT_LIT, TOKEN_FLOAT_LIT, TOKEN_OPEN_PAREN))
        return NULL;

    AST *left = (AST *)malloc(sizeof(AST));

    switch (tok.type) {
        case TOKEN_INT_LIT:
            left->type = AST_INT_LIT;
            left->int_lit = tok.int_lit;
            break;
        case TOKEN_FLOAT_LIT:
            left->type = AST_FLOAT_LIT;
            left->float_lit = tok.float_lit;
            break;
        case TOKEN_OPEN_PAREN:
            free(left);
            left = parse_expression(parser, 0);
            if (parser->error) return NULL;

            if (!expect_next_token(parser, &tok, TOKEN_CLOSE_PAREN)) return NULL;
            break;
    }

    while (get_token_type_precedence(peek_next_token_type(parser)) > min_prec) {
        AST *op = (AST *)malloc(sizeof(AST));
        op->type = AST_BINARY_OP;
        op->left = left;
        tok = *get_next_token(parser);
        switch (tok.type) {
            case TOKEN_PLUS:
                op->op = OP_PLUS;
                break;
            case TOKEN_MINUS:
                op->op = OP_MINUS;
                break;
            case TOKEN_MULTIPLY:
                op->op = OP_MULTIPLY;
                break;
            case TOKEN_DIVIDE:
                op->op = OP_DIVIDE;
                break;
        }

        op->right = parse_expression(parser, get_token_type_precedence(tok.type));
        if (parser->error) return NULL;

        left = op;
    }

    return left;
}

AST *parse_statement(ParserState *parser) {
    Token tok;

    // NOTE: changed to calloc here because array macros need array to be initialized to all zero
    AST *statement = (AST *)calloc(1, sizeof(AST));

    if (match_next_token(parser, &tok, TOKEN_OPEN_CURLY)) {
        statement->type = AST_BLOCK;

        while (!match_next_token(parser, &tok, TOKEN_CLOSE_CURLY)) {
            AST *s = parse_statement(parser);
            array_append(statement->block, s);
        }

        expect_next_token(parser, &tok, TOKEN_CLOSE_CURLY);

    } else if (match_next_token(parser, &tok, TOKEN_IDENTIFIER)) {
        Token ident = tok;
        if (match_next_token(parser, &tok, TOKEN_COLON)) {
            if (!match_next_token(parser, &tok, TOKEN_IDENTIFIER)) return NULL;
            if (!match_next_token(parser, &tok, TOKEN_EQUALS)) return NULL;

            statement->type = AST_DECLARATION;
            statement->symbol = ident.identifier;
            statement->expr = parse_expression(parser, 0);
        } else if (match_next_token(parser, &tok, TOKEN_COLON_EQUALS)) {
            statement->type = AST_DECLARATION;
            statement->symbol = ident.identifier;
            statement->expr = parse_expression(parser, 0);
        }
    } else if (match_next_token(parser, &tok, TOKEN_RETURN)) {
        statement->type = AST_RETURN;
        statement->expr = parse_expression(parser, 0);
    } else {
        statement = parse_expression(parser, 0);
    }

    return statement;
}

AST *parse_program(ParserState *parser) {
    AST *ret = parse_statement(parser);
    if (parser->error) return NULL;

    // TODO: I'm not sure if it's better to pass pointer to local tok
    // or have tok be a global variable, but switching to local pointer
    // cleaned up parser code a lot from previous solution
    Token tok;
    if (!expect_next_token(parser, &tok, TOKEN_EOF)) {
        free(ret);
        return NULL;
    }

    return ret;
}
