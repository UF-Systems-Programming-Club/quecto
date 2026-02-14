#ifndef LEXER_H
#define LEXER_H

#include "common.h"

typedef enum {
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_SEMICOLON,
    TOKEN_DIVIDE,
    TOKEN_INT_LIT,
    TOKEN_FLOAT_LIT,
    TOKEN_EOF,
    TOKEN_OPEN_CURLY,
    TOKEN_CLOSE_CURLY,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_IDENTIFIER,
    TOKEN_COLON,
    TOKEN_EQUALS,
    TOKEN_COLON_EQUALS,
    TOKEN_RETURN,
    TOKEN_ERROR,

    TOKEN_COUNT // Make sure this token is the last one
                // listed in the enum, as this is assumed by compile time assertions
} TokenType;

typedef struct {
    TokenType type;
    union {
        unsigned int int_lit;
        float float_lit;
        char *identifier;
    };
    unsigned int row, col;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenArray;

extern const char *token_to_string_table[];
int int_from_str(const char* a, size_t len);
float float_from_str(const char* a, size_t len);
bool is_number(char c);
bool is_alpha(char c);
void print_token(Token tok);
TokenArray tokenize(const char* buf, size_t buf_size);

#endif
