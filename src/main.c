#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "AST.h"

int main() {
    FILE *f = fopen("hello.q", "r");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);

    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(fsize + 1);
    fread(buf, 1, fsize, f);
    fclose(f);

    buf[fsize] = '\0';
    size_t buf_size = fsize;

    printf("%s\n", buf);

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
            case '(':
                tok.type = TOKEN_OPEN_PAREN;
                array_append(tokens, tok);
                break;
            case ')':
                tok.type = TOKEN_CLOSE_PAREN;
                array_append(tokens, tok);
                break;

            default:
                if (is_number(c)) {
                    tok.type = TOKEN_INT_LIT;
                    int num_decimal_points = 0;
                    while (is_number(buf[next]) || buf[next] == '.') {
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

    for (int i = 0; i < tokens.count; i++) {
        print_token(tokens.items[i]);
    }
    printf("\n");

    ParserState parser = {0};
    parser.tokens = tokens;

    AST *ast = parse_program(&parser);

    if (!parser.error) {
        AST result = evaluate_ast(ast);
        switch (result.type) {
            case AST_BINARY_OP:
                printf("error");
                break;
            case AST_FLOAT_LIT:
                printf("float result: %f\n", result.float_lit);
                break;
            case AST_INT_LIT:
                printf("unsigned result: %u\n", result.int_lit);
                break;
        }
    }
}