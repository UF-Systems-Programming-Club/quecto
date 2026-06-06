#ifndef LEXER_H
#define LEXER_H

#include "common.h"

#define MAX_KEYWORD_LEN 128

typedef enum {
    TOKEN_NONE,
    TOKEN_CLOSE_CURLY, TOKEN_OPEN_CURLY,   TOKEN_PLUS,  TOKEN_MULTIPLY,
    TOKEN_SEMICOLON,   TOKEN_OPEN_PAREN,   TOKEN_MINUS, TOKEN_DIVIDE,
    TOKEN_INT_LIT,     TOKEN_CLOSE_PAREN,  TOKEN_COMMA, TOKEN_AMPERSAND,
    TOKEN_FLOAT_LIT,   TOKEN_OPEN_SQUARE,  TOKEN_PERIOD, 
    TOKEN_STR_LIT,     TOKEN_CLOSE_SQUARE, TOKEN_CARET,
    TOKEN_EOF,         TOKEN_IDENTIFIER,   TOKEN_COLON,
    
    TOKEN_EQUALS, TOKEN_EQUALS_EQUALS,
    TOKEN_LESS_EQUALS, TOKEN_GREATER_EQUALS,
    TOKEN_LESS_THAN,   TOKEN_GREATER_THAN,
    
    TOKEN_IF, TOKEN_ELIF, TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_PROC, TOKEN_ARROW, TOKEN_EXTERN,
    TOKEN_RETURN,

    TOKEN_U8, TOKEN_I8,
    TOKEN_U32, TOKEN_I32,

    TOKEN_COUNT // Make sure this token is the last one
                // listed in the enum, as this is assumed by compile time assertions
} TokenType;

typedef struct {
    TokenType type;
    union {
        unsigned int int_lit;
        float float_lit;
        char *string_lit;
        char *identifier;
    };
    unsigned int line, col;
    StringView lexeme;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenArray;

typedef struct keyword {
    const char *name;
    int token;
} KeywordEntry;

extern const char *token_to_string_table[];
int int_from_str(const char* a, size_t len);
float float_from_str(const char* a, size_t len);
void print_token(Token tok);
TokenArray tokenize(Arenas *arena, const char *buf, size_t size);


KeywordEntry *lookup_keyword(const char *str, size_t len);

#endif
