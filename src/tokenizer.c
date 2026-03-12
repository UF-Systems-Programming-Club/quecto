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
    [TOKEN_OPEN_CURLY] = "{",
    [TOKEN_CLOSE_CURLY] = "}",
    [TOKEN_OPEN_PAREN] = "(",
    [TOKEN_CLOSE_PAREN] = ")",
    [TOKEN_SEMICOLON] = ";",
    [TOKEN_COLON] = ":",
    [TOKEN_COMMA] = ",",
    [TOKEN_EQUALS] = "=",
    [TOKEN_EQUALS_EQUALS] = "==",
    [TOKEN_ARROW] = "=>",
    [TOKEN_LESS_EQUALS] = "<=",
    [TOKEN_GREATER_EQUALS] = ">=",
    [TOKEN_LESS_THAN] = "<",
    [TOKEN_GREATER_THAN] = ">",
    [TOKEN_INT_LIT] = "integer literal",
    [TOKEN_FLOAT_LIT] = "float literal",
    [TOKEN_IDENTIFIER] = "identifier",
    [TOKEN_RETURN] = "return",
    [TOKEN_IF] = "if",
    [TOKEN_ELIF] = "elif",
    [TOKEN_ELSE] = "else",
    [TOKEN_WHILE] = "while",
    [TOKEN_PROC] = "proc",
    [TOKEN_EOF] = "end of file"
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

bool is_number(char c) {
    return '0' <= c && c <= '9';
}

bool is_alpha(char c) {
    return ('a' <= c  && c <= 'z') || ('A' <= c && c <= 'Z');
}

void print_token(Token tok) {
    switch (tok.type) {
        case TOKEN_PLUS:            printf("+"); break;
        case TOKEN_MINUS:           printf("-"); break;
        case TOKEN_MULTIPLY:        printf("*"); break;
        case TOKEN_DIVIDE:          printf("/"); break;
        case TOKEN_SEMICOLON:       printf(";"); break;
        case TOKEN_INT_LIT:         printf("%u", tok.int_lit); break;
        case TOKEN_FLOAT_LIT:       printf("%.2f", tok.float_lit); break;
        case TOKEN_EOF:             printf("EOF"); break;
        case TOKEN_OPEN_PAREN:      printf("("); break;
        case TOKEN_CLOSE_PAREN:     printf(")"); break;
        case TOKEN_OPEN_CURLY:      printf("{"); break;
        case TOKEN_CLOSE_CURLY:     printf("}"); break;
        case TOKEN_IDENTIFIER:      printf("%s", tok.identifier); break;
        case TOKEN_COLON:           printf(":"); break;
        case TOKEN_COMMA:           printf(","); break;
        case TOKEN_EQUALS:          printf("="); break;
        case TOKEN_EQUALS_EQUALS:   printf("=="); break;
        case TOKEN_LESS_EQUALS:     printf("<="); break;
        case TOKEN_GREATER_EQUALS:  printf(">="); break;
        case TOKEN_LESS_THAN:       printf("<"); break;
        case TOKEN_GREATER_THAN:    printf(">"); break;
        case TOKEN_RETURN:          printf("return"); break;
        case TOKEN_IF:              printf("if"); break;
        case TOKEN_ELIF:            printf("elif"); break;
        case TOKEN_ELSE:            printf("else"); break;
        case TOKEN_WHILE:           printf("while"); break;
        case TOKEN_PROC:       printf("proc"); break;
        case TOKEN_ARROW:           printf("=>"); break;
        default:                    UNREACHABLE("TokenType");
    }
    printf("\n");
}

TokenArray tokenize(const char *buf, size_t buf_size) {
    TokenArray tokens = {0};

    // TODO: should have an array or hashmap of keywords to make
    // keyword recognization faster and smaller

    size_t start = 0;
    size_t next = 0;
    size_t line = 1;
    size_t column = 1;

    while (start < buf_size) {
        char c = buf[next++];

        Token tok = {
            .col = column,
            .line = line,
        };

        switch (c) {
            case '\n':
                column = 1;
                line++;
                start = next;
                continue;
            case ' ':
            case '\t':
                column++;
                start = next;
                continue;
            case '\r':
                UNREACHABLE("idk what to do here yet");
                break;
            case '+': tok.type = TOKEN_PLUS;        break;
            case '-': tok.type = TOKEN_MINUS;       break;
            case '*': tok.type = TOKEN_MULTIPLY;    break;
            case '/': tok.type = TOKEN_DIVIDE;      break;
            case '{': tok.type = TOKEN_OPEN_CURLY;  break;
            case '}': tok.type = TOKEN_CLOSE_CURLY; break;
            case '(': tok.type = TOKEN_OPEN_PAREN;  break;
            case ')': tok.type = TOKEN_CLOSE_PAREN; break;
            case ';': tok.type = TOKEN_SEMICOLON;   break;
            case ':': tok.type = TOKEN_COLON;       break;
            case ',': tok.type = TOKEN_COMMA;       break;
            case '<':
                switch (buf[next]) {
                    case '=':   tok.type = TOKEN_LESS_EQUALS; next++; break;
                    default:    tok.type = TOKEN_LESS_THAN;           break;
                }
                break;
            case '>':
                switch (buf[next]) {
                    case '=':   tok.type = TOKEN_GREATER_EQUALS; next++; break;
                    default:    tok.type = TOKEN_GREATER_THAN;           break;
                }
                break;
            case '=':
                switch (buf[next]) {
                    case '=':   tok.type = TOKEN_EQUALS_EQUALS; next++; break;
                    case '>':   tok.type = TOKEN_ARROW;         next++; break;
                    default:    tok.type = TOKEN_EQUALS;                break;
                }
                break;
            default:
                if (is_alpha(c)) {
                    tok.type = TOKEN_IDENTIFIER;
                    while (next < buf_size && (is_alpha(buf[next]) || is_number(buf[next]) || buf[next] == '_')) {
                        next++;
                    }

                    char keyword[MAX_KEYWORD_LEN];
                    memcpy(keyword,  &buf[start], next - start);
                    keyword[next - start] = '\0';
                    
                    struct keyword *result = lookup_keyword(keyword, next - start);
                    if (result != NULL) {
                        tok.type = result->token;
                    } else {
                        tok.identifier = (char *)malloc(next - start + 1);
                        strncpy(tok.identifier, &buf[start], next - start);
                        tok.identifier[next - start] = '\0';
                    }
                    
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
                        report_error(tok.line, tok.col, "too many decimal points in float literal");

                    switch (tok.type) {
                        case TOKEN_FLOAT_LIT:
                            tok.float_lit = float_from_str(&buf[start], next - start);
                            break;
                        case TOKEN_INT_LIT:
                            tok.int_lit = int_from_str(&buf[start], next - start);
                            break;
                        default:
                            UNREACHABLE("TokenType");
                    }
                } else {
                    report_error(tok.line, tok.col, "unrecognized character \'%c\'", c);
                }
                break;
        }

        array_append(tokens, tok);
        column += next - start;
        start = next;
    }

    Token tok_eof = {0};
    tok_eof.type = TOKEN_EOF;
    array_append(tokens, tok_eof);

    return tokens;
}
