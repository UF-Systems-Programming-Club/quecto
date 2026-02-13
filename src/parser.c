#include "parser.h"
#include "AST.h"
#include "common.h"
#include "tokenizer.h"

int get_token_precedence_table[] = {
    [TOKEN_PLUS] = 1,
    [TOKEN_MINUS] = 1,
    [TOKEN_MULTIPLY] = 2,
    [TOKEN_DIVIDE] = 2,
    [TOKEN_ASSIGN] = 1,
};

bool token_is_operator(TokenType type) {
    return type == TOKEN_PLUS     ||
           type == TOKEN_MINUS    ||
           type == TOKEN_MULTIPLY ||
           type == TOKEN_ASSIGN   ||
           type == TOKEN_DIVIDE;
}

int get_token_precedence(TokenType type) {
    if (token_is_operator(type)) return get_token_precedence_table[type];
    return -1;
}

int print_parser_error(ParserState *parser, const char *format, ...) {
    va_list args;
    va_start(args, format);

    Token *tok = &parser->tokens.items[parser->current];
    printf("parser error at (line: %d, column: %d): ", tok->row, tok->col);
    int result = vprintf(format, args);

    va_end(args);
    return result;
}

TokenType peek_next_token(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return TOKEN_EOF;

    return parser->tokens.items[parser->current].type;
}

bool check_next_token(ParserState *parser, TokenType type) {
    return (peek_next_token(parser) == type);
}

void eat_next_token(ParserState *parser) {
    if (parser->current < parser->tokens.count) parser->current++;
}

bool match_next_token(ParserState *parser, TokenType type) {
    if (peek_next_token(parser) == type) {
        return true;
    } else {
        print_parser_error(parser,"expected \"%s\" but got \"%s\" instead\n",
            token_to_string_table[type],
            token_to_string_table[peek_next_token(parser)]);
        parser->error = true;
        return false;
    }
}

bool match_next_token_multiple(ParserState *parser, int count, ...) {
    assert(count > 1 && "Just use match_next_token if you're only matching against one token type");
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        TokenType type = va_arg(args, TokenType);
        if (peek_next_token(parser) == type) return true;
    }
    va_end(args);
    va_start(args, count);

    parser->error = true;
    print_parser_error(parser, "");
    for (int i = 0; i < count - 1; i++) {
        printf("\"%s\", ", token_to_string_table[va_arg(args, TokenType)]);
    }
    printf("or \"%s\", ", token_to_string_table[va_arg(args, TokenType)]);
    printf("but got \"%s\" instead\n", token_to_string_table[peek_next_token(parser)]);

    va_end(args);
    return false;
}

Token get_next_token(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return parser->tokens.items[parser->tokens.count - 1];;
    return parser->tokens.items[parser->current++];
}

AST *parse_expression(ParserState *parser, int min_prec) {
    if (!match_next_token_multiple(parser, 3, TOKEN_FLOAT_LIT, TOKEN_INT_LIT, TOKEN_OPEN_PAREN))
        return NULL;

    Token tok = get_next_token(parser);

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

            if (!match_next_token(parser, TOKEN_CLOSE_PAREN)) return NULL;
            get_next_token(parser);
            break;
        case TOKEN_ASSIGN:
            left->type = AST_BINARY_OP;
            left->op = OP_ASSIGN;
            break;
    }

    while (get_token_precedence(peek_next_token(parser)) > min_prec) {
        AST *op = (AST *)malloc(sizeof(AST));
        op->type = AST_BINARY_OP;
        op->left = left;
        tok = get_next_token(parser);
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
            case TOKEN_ASSIGN:
                op->op = OP_ASSIGN;
                break;
            }

        op->right = parse_expression(parser, get_token_precedence(tok.type));
        if (parser->error) return NULL;

        left = op;
    }

    return left;
}

AST *parse_statement(ParserState *parser) {

    AST *statement = (AST *)malloc(sizeof(AST));

    if (check_next_token(parser, TOKEN_OPEN_CURLY)) {
        eat_next_token(parser);
        statement->type = AST_BLOCK;
        print_token((Token){.type = peek_next_token(parser)});

        while (!check_next_token(parser, TOKEN_CLOSE_CURLY)) {
            AST* s = parse_statement(parser);
            array_append(statement->block, s);
        }

        match_next_token(parser, TOKEN_CLOSE_CURLY);
        eat_next_token(parser);
        print_token((Token){.type = peek_next_token(parser)});;

    } else if (check_next_token(parser, TOKEN_IDENTIFIER)) {
        Token identity = get_next_token(parser);

        if (check_next_token(parser, TOKEN_COLON)) {
            eat_next_token(parser);
            if (check_next_token(parser, TOKEN_IDENTIFIER)) {
                eat_next_token(parser);
                if (check_next_token(parser, TOKEN_EQUALS)) {
                    eat_next_token(parser);
                    statement->type = AST_DECLARATION;
                    statement->symbol = identity.identifier;
                    statement->expr = parse_expression(parser, 0);
                }
            }
        } else if (check_next_token(parser, TOKEN_COLON_EQUALS)) {
            eat_next_token(parser);
            parse_expression(parser, 0);
        }
    } else if (check_next_token(parser, TOKEN_RETURN)) {

    } else {
        statement = parse_expression(parser, 0);
    }

    return statement;
}

AST *parse_program(ParserState *parser) {
    AST *ret = parse_statement(parser);
    if (parser->error) return NULL;

    if (!match_next_token(parser, TOKEN_EOF)) {
        free(ret);
        return NULL;
    }

    return ret;
}
