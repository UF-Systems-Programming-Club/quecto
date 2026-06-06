#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tokenizer.h"
#include "common.h"
#include "error.h"

const char *token_to_string_table[] = {
    [TOKEN_PLUS] = "+",
    [TOKEN_MINUS] = "-",
    [TOKEN_MULTIPLY] = "*",
    [TOKEN_DIVIDE] = "/",
    [TOKEN_CARET] = "^",
    [TOKEN_AMPERSAND] = "&",
    [TOKEN_OPEN_CURLY] = "{",
    [TOKEN_CLOSE_CURLY] = "}",
    [TOKEN_OPEN_PAREN] = "(",
    [TOKEN_CLOSE_PAREN] = ")",
    [TOKEN_OPEN_SQUARE] = "[",
    [TOKEN_CLOSE_SQUARE] = "]",
    [TOKEN_SEMICOLON] = ";",
    [TOKEN_COLON] = ":",
    [TOKEN_PERIOD] = ".",
    [TOKEN_COMMA] = ",",
    [TOKEN_EQUALS] = "=",
    [TOKEN_EQUALS_EQUALS] = "==",
    [TOKEN_ARROW] = "=>",
    [TOKEN_LESS_EQUALS] = "<=",
    [TOKEN_GREATER_EQUALS] = ">=",
    [TOKEN_LESS_THAN] = "<",
    [TOKEN_GREATER_THAN] = ">",
    [TOKEN_INT_LIT] = "integer literal",
    [TOKEN_STR_LIT] = "string literal",
    [TOKEN_FLOAT_LIT] = "float literal",
    [TOKEN_IDENTIFIER] = "identifier",
    [TOKEN_RETURN] = "return",
    [TOKEN_IF] = "if",
    [TOKEN_ELIF] = "elif",
    [TOKEN_ELSE] = "else",
    [TOKEN_WHILE] = "while",
    [TOKEN_PROC] = "proc",
    [TOKEN_EXTERN] = "extern",
    [TOKEN_U32] = "u32",
    [TOKEN_I32] = "i32",
    [TOKEN_I8] = "i8",
    [TOKEN_U8] = "u8",
    [TOKEN_EOF] = "end of file"
};

const TokenType token_from_char[] = {
    [':'] = TOKEN_COLON,
    [';'] = TOKEN_SEMICOLON,
    ['+'] = TOKEN_PLUS,
    ['-'] = TOKEN_MINUS,
    ['/'] = TOKEN_DIVIDE,
    ['*'] = TOKEN_MULTIPLY,
    ['='] = TOKEN_EQUALS,
    ['>'] = TOKEN_GREATER_THAN,
    ['<'] = TOKEN_LESS_THAN,
    ['('] = TOKEN_OPEN_PAREN,
    [')'] = TOKEN_CLOSE_PAREN,
    ['['] = TOKEN_OPEN_SQUARE,
    [']'] = TOKEN_CLOSE_SQUARE,
    ['{'] = TOKEN_OPEN_CURLY,
    ['}'] = TOKEN_CLOSE_CURLY,
    ['.'] = TOKEN_PERIOD,
    [','] = TOKEN_COMMA,
    ['^'] = TOKEN_CARET,
    ['&'] = TOKEN_AMPERSAND,
};

static_assert(sizeof(token_to_string_table) / sizeof(char *) == TOKEN_COUNT,
              "Every token must have a corresponding entry in the token to string table, so add an entry probably");

int int_from_str(const char *a, size_t len) {
    int tens = 1;
    int accum = 0;
    for (int i = len - 1; i >= 0; i--) {
        accum += (a[i] - '0') * tens;
        tens *= 10;
    }
    return accum;
}

float float_from_str(const char *a, size_t len) {
    float accum = 0;
    int decimal = len/*, exponent = -1*/;
    float tens = 1;

    for (int i = 0; i < len; i++) {
        if (a[i] == '.') {
            decimal = i;
        }
        if (a[i] == 'e' || a[i] == 'E') {
            // exponent = i; // TODO : ADD EXPONENT support
        }
    }

    for (int i = decimal - 1; i >= 0; i--) {
        accum += (a[i] - '0') * tens;
        tens *= 10;
    }

    tens = 0.1;
    for (int i = decimal + 1; i < len; i++) {
        accum += (a[i] - '0') * tens;
        tens /= 10;
    }

    return accum;
}

bool is_number(uint8_t c) {
    return '0' <= c && c <= '9';
}

bool is_alpha(uint8_t c) {
    return ('a' <= c  && c <= 'z') || ('A' <= c && c <= 'Z');
}

void print_token(Token tok) {
    switch (tok.type) {
        case TOKEN_INT_LIT: printf("%u", tok.int_lit); break;
        case TOKEN_FLOAT_LIT: printf("%.2f", tok.float_lit); break;
        case TOKEN_IDENTIFIER: printf("%s", tok.identifier); break;
        default: printf("%s", token_to_string_table[tok.type]); break;
    }
    printf("\n");
}

TokenArray tokenize(Arenas *arena, const char *buf, size_t size) {
    TokenArray tokens = { 0 };

    int mark = arena_mark(arena->scratch);

    size_t start = 0;
    size_t next = 0;
    size_t col = 1;
    size_t line = 0;
    
    while (start < size) {
        uint8_t chr = buf[next++];
        Token tok = { .col = col, .line = line };

        switch (chr) {
            case '\n':
                col = 1;
                line++;
                start = next;
                continue;
            case ' ':
            case '\t':
                col++;
                start = next;
                continue;
            case '\r':
                col = 1;
                line++;
                start = buf[next] == '\n' ? ++next : next;
                continue;
            case '/':
                if (buf[next] == '/') {
                    while (next < size && buf[next] != '\n')
                        next++;
                    continue;
                }
            case '+': case '*': case '-': case '^': case '&': case '{': case '}': case '(': case ')': case '[': case ']':  case ':': case ';': case '.': case ',':
                tok.type = token_from_char[chr];
                break;
            case '=':
                tok.type = token_from_char[chr];
                if (buf[next] == '>' || buf[next] == '=') next++;
                break;
            case '<': case '>':
                tok.type = token_from_char[chr];
                if (buf[next] == '=') next++;
                break;
            default: {
                if (is_alpha(chr) || chr == '_') {
                    tok.type = TOKEN_IDENTIFIER;
                    while (next < size && (is_alpha(buf[next]) || is_number(buf[next]) || buf[next] == '_')) {
                        next++;
                    }
                } else if (is_number(chr)) {
                    tok.type = TOKEN_INT_LIT;
                    int num_decimal_points = 0;
                    while (next < size && (is_number(buf[next]) || buf[next] == '.')) {
                        if (buf[next] == '.') {
                            tok.type = TOKEN_FLOAT_LIT;
                            num_decimal_points++;
                        }
                        next++;
                    }

                    if (num_decimal_points > 1)
                        report_error(tok.line, tok.col, "too many decimal points in float literal");
                }
            }
        }

        char *token = arena_alloc(arena->scratch, sizeof(char) * (next - start));
        strncpy(token, &buf[start], next - start);

        if (next - start > 1) {
            struct keyword *result = lookup_keyword(token, next - start);
            if (result != NULL)
                tok.type = result->token;
        }

        switch (tok.type) {
            case TOKEN_IDENTIFIER:
                tok.identifier = arena_alloc(arena->persistent, sizeof(char) * (next - start + 1));
                strncpy(tok.identifier, token, next - start);
                tok.identifier[next - start] = '\0';
                break;
            case TOKEN_INT_LIT:
                tok.int_lit = int_from_str(&buf[start], next - start);
                break;
            case TOKEN_FLOAT_LIT:
                tok.float_lit = float_from_str(&buf[start], next - start);
                break;
            case TOKEN_NONE:
                report_error(line, col, "unrecognized token");
                break;
            default:
                break;
        }

        tok.lexeme.len = next - start;
        tok.lexeme.str = &buf[start];
        arena_array_append(arena->persistent, tokens, tok);
        col += next - start;
        start = next;
    }

    Token tok_eof = {0};
    tok_eof.type = TOKEN_EOF;
    arena_array_append(arena->persistent, tokens, tok_eof);

    arena_restore(arena->scratch, mark);

    return tokens;
}
