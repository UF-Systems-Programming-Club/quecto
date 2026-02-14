#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tokenizer.h"
#include "common.h"

const char *token_to_string_table[] = {
    [TOKEN_PLUS] = "+",
    [TOKEN_MINUS] = "-",
    [TOKEN_MULTIPLY] = "*",
    [TOKEN_DIVIDE] = "/",
    [TOKEN_OPEN_CURLY] = "{",
    [TOKEN_CLOSE_CURLY] = "}",
    [TOKEN_OPEN_PAREN] = "(",
    [TOKEN_CLOSE_PAREN] = ")",
    [TOKEN_SEMICOLON] = ";",
    [TOKEN_COLON] = ":",
    [TOKEN_EQUALS] = "=",
    [TOKEN_COLON_EQUALS] = ":=",
    [TOKEN_INT_LIT] = "integer literal",
    [TOKEN_FLOAT_LIT] = "float literal",
    [TOKEN_IDENTIFIER] = "identifier",
    [TOKEN_RETURN] = "return",
    [TOKEN_ERROR] = "error",
    [TOKEN_EOF] = "end of file"
};

static_assert(sizeof(token_to_string_table) / sizeof(char *) == TOKEN_COUNT,
              "Every token must have a corresponding entry in the token to string table, so add an entry probably");

int int_from_str(const char* a, size_t len) {
    int tens = 1;
    int accum = 0;
    for (int i = len - 1; i >= 0; i--) {
        accum += (a[i] - '0') * tens;
        tens *= 10;
    }
    return accum;
}

float float_from_str(const char* a, size_t len) {
    float accum = 0;
    int decimal = len, exponent = -1;
    float tens = 1;

    for (int i = 0; i < len; i++) {
        if (a[i] == '.') {
            decimal = i;
        }
        if (a[i] == 'e' || a[i] == 'E') {
            exponent = i; // TODO : ADD EXPONENT support
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

bool is_number(char c) {
    return '0' <= c && c <= '9';
}

bool is_alpha(char c) {
    return ('a' <= c  && c <= 'z') || ('A' <= c && c <= 'Z');
}

void print_token(Token tok) {
    switch (tok.type) {
        case TOKEN_PLUS:         printf("+\n"); break;
        case TOKEN_MINUS:        printf("-\n"); break;
        case TOKEN_MULTIPLY:     printf("*\n"); break;
        case TOKEN_DIVIDE:       printf("/\n"); break;
        case TOKEN_SEMICOLON:    printf(";\n"); break;
        case TOKEN_INT_LIT:      printf("%u\n", tok.int_lit); break;
        case TOKEN_FLOAT_LIT:    printf("%.2f\n", tok.float_lit); break;
        case TOKEN_EOF:          printf("EOF\n"); break;
        case TOKEN_OPEN_PAREN:   printf("(\n"); break;
        case TOKEN_OPEN_CURLY:   printf("{\n"); break;
        case TOKEN_CLOSE_CURLY:  printf("}\n"); break;
        case TOKEN_IDENTIFIER:   printf("%s\n", tok.identifier); break;
        case TOKEN_COLON:        printf(":\n"); break;
        case TOKEN_EQUALS:       printf("=\n"); break;
        case TOKEN_COLON_EQUALS: printf(":=\n"); break;
        case TOKEN_RETURN:       printf("return\n"); break;
        case TOKEN_CLOSE_PAREN:  printf(")\n"); break;

        default:                assert(0 && "Every token needs to be able to be printed, so add entry");
    }
}

TokenArray tokenize(const char* buf, size_t buf_size) {
    TokenArray tokens = {0};

    size_t start = 0;
    size_t next = 0;
    size_t row = 1;
    size_t column = 1;

    while (start < buf_size) {
        char c = buf[next++];
        size_t column_width = 1;

        Token tok = {
            .col = column,
            .row = row,
        };

        switch (c) {
            case '\n':
                column = 1;
                row++;
                break;
            case ' ':
            case '\t':
            case '\r':
                break;
            case '+':
                tok.type = TOKEN_PLUS;
                array_append(tokens, tok);
                break;
            case '-':
                tok.type = TOKEN_MINUS;
                array_append(tokens, tok);
                break;
            case '*':
                tok.type = TOKEN_MULTIPLY;
                array_append(tokens, tok);
                break;
            case '/':
                tok.type = TOKEN_DIVIDE;
                array_append(tokens, tok);
                break;
            case '{':
                tok.type = TOKEN_OPEN_CURLY;
                array_append(tokens, tok);
                break;
            case '}':
                tok.type = TOKEN_CLOSE_CURLY;
                array_append(tokens, tok);
                break;
            case '(':
                tok.type = TOKEN_OPEN_PAREN;
                array_append(tokens, tok);
                break;
            case ')':
                tok.type = TOKEN_CLOSE_PAREN;
                array_append(tokens, tok);
                break;
            case ':':
                if (next < buf_size && buf[next] == '=') {
                    next++;
                    tok.type = TOKEN_COLON_EQUALS;
                } else {
                    tok.type = TOKEN_COLON;
                }
                array_append(tokens, tok);
                break;
            case '=':
                tok.type = TOKEN_EQUALS;
                array_append(tokens, tok);
                break;
            default:
                if (is_alpha(c)) {
                    tok.type = TOKEN_IDENTIFIER;
                    while (next < buf_size && (is_alpha(buf[next]) || is_number(buf[next]) || buf[next] == '_')) {
                        next++;
                    }

                    if (strncmp(&buf[start], "return", next - start) == 0) {
                        tok.type = TOKEN_RETURN;
                    } else {
                        tok.identifier = (char*) malloc(next - start + 1);
                        strncpy(tok.identifier, &buf[start], next - start);
                        tok.identifier[next - start] = '\0';
                    }

                    array_append(tokens, tok);
                }
                else if (is_number(c)) {
                    tok.type = TOKEN_INT_LIT;
                    int num_decimal_points = 0;
                    while (next < buf_size && (is_number(buf[next]) || buf[next] == '.')) {
                        if (buf[next] == '.') {
                            tok.type = TOKEN_FLOAT_LIT;
                            num_decimal_points++;
                        }
                        next++;
                    }

                    if (num_decimal_points > 1)
                        printf("tokenizing error: too many decimal points in float literal\n");

                    switch (tok.type) {
                        case TOKEN_FLOAT_LIT:
                            tok.float_lit = float_from_str(&buf[start], next - start);
                            break;
                        case TOKEN_INT_LIT:
                            tok.int_lit = int_from_str(&buf[start], next - start);
                            break;
                    }

                    column_width = next - start;
                    array_append(tokens, tok);
                } else {
                    printf("tokenizing error: unrecognized character \"%c\"\n", c);
                }
                break;
        }

        column += column_width;
        start = next;
    }

    Token tok_eof = {0};
    tok_eof.type = TOKEN_EOF;
    array_append(tokens, tok_eof);

    return tokens;
}
