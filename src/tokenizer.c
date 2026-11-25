#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tokenizer.h"
#include "common.h"
#include "error.h"

#define TOKEN_INFO(ident, tk_flags, prec) (TokenInfo) {\
    .name = SV_LITERAL(ident),\
    .flags = tk_flags,\
    .precedence = prec\
}

const TokenInfo token_info_table[] = {
    [TOKEN_PLUS]           = TOKEN_INFO("+", TOK_OPERATOR, 2),
    [TOKEN_MINUS]          = TOKEN_INFO("-", TOK_OPERATOR, 2),
    [TOKEN_MULTIPLY]       = TOKEN_INFO("*", TOK_OPERATOR, 3),
    [TOKEN_DIVIDE]         = TOKEN_INFO("/", TOK_OPERATOR, 3),
    [TOKEN_CARET]          = TOKEN_INFO("^", TOK_OPERATOR, 7),
    [TOKEN_AMPERSAND]      = TOKEN_INFO("&", TOK_OPERATOR, 7),
    [TOKEN_OPEN_CURLY]     = TOKEN_INFO("{", 0, -1),
    [TOKEN_CLOSE_CURLY]    = TOKEN_INFO("}", 0, -1),
    [TOKEN_OPEN_PAREN]     = TOKEN_INFO("(", TOK_HAS_PREC, 4),
    [TOKEN_CLOSE_PAREN]    = TOKEN_INFO(")", 0, -1),
    [TOKEN_OPEN_SQUARE]    = TOKEN_INFO("[", TOK_HAS_PREC, 4),
    [TOKEN_CLOSE_SQUARE]   = TOKEN_INFO("]", 0, -1),
    [TOKEN_SEMICOLON]      = TOKEN_INFO(";", 0, -1),
    [TOKEN_COLON]          = TOKEN_INFO(":", 0, -1),
    [TOKEN_PERIOD]         = TOKEN_INFO(".", TOK_OPERATOR, 5),
    [TOKEN_COMMA]          = TOKEN_INFO(",", 0, -1),
    [TOKEN_EQUALS]         = TOKEN_INFO("=", 0, -1),
    [TOKEN_EQUALS_EQUALS]  = TOKEN_INFO("==", TOK_OPERATOR, 1),
    [TOKEN_ARROW]          = TOKEN_INFO("=>", 0, -1),
    [TOKEN_LESS_EQUALS]    = TOKEN_INFO("<=", TOK_OPERATOR, 1),
    [TOKEN_GREATER_EQUALS] = TOKEN_INFO(">=", TOK_OPERATOR, 1),
    [TOKEN_LESS_THAN]      = TOKEN_INFO("<", TOK_OPERATOR, 1),
    [TOKEN_GREATER_THAN]   = TOKEN_INFO(">", TOK_OPERATOR, 1),
    [TOKEN_INT_LIT]        = TOKEN_INFO("int lit", TOK_HAS_PREC, 6),
    [TOKEN_STR_LIT]        = TOKEN_INFO("str lit", TOK_HAS_PREC, 6),
    [TOKEN_FLOAT_LIT]      = TOKEN_INFO("float lit", TOK_HAS_PREC, 6),
    [TOKEN_IDENTIFIER]     = TOKEN_INFO("identifier", TOK_HAS_PREC, 6),
    [TOKEN_RETURN]         = TOKEN_INFO("return", TOK_KEYWORD, -1),
    [TOKEN_IF]             = TOKEN_INFO("if", TOK_KEYWORD, -1),
    [TOKEN_ELIF]           = TOKEN_INFO("elif", TOK_KEYWORD, -1),
    [TOKEN_ELSE]           = TOKEN_INFO("else", TOK_KEYWORD, -1),
    [TOKEN_WHILE]          = TOKEN_INFO("while", TOK_KEYWORD, -1),
    [TOKEN_PROC]           = TOKEN_INFO("proc", TOK_KEYWORD, -1),
    [TOKEN_EXTERN]         = TOKEN_INFO("extern", TOK_KEYWORD, -1),
    [TOKEN_U32]            = TOKEN_INFO("u32", TOK_PRIMITIVE, -1),
    [TOKEN_I32]            = TOKEN_INFO("i32", TOK_PRIMITIVE, -1),
    [TOKEN_I8]             = TOKEN_INFO("i8", TOK_PRIMITIVE, -1),
    [TOKEN_U8]             = TOKEN_INFO("u8", TOK_PRIMITIVE, -1),
    [TOKEN_EOF]            = TOKEN_INFO("eof", 0, -1),
};

#undef TOKEN_INFO

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

static_assert(sizeof(token_info_table) / sizeof(TokenInfo) == TOKEN_COUNT,
              "Every token must have a corresponding entry in the token to string table, so add an entry probably");

int int_from_str(const char *a, size_t len) {
    int tens = 1;
    int accum = 0;
    for (size_t i = len; i-- > 0;) {
        accum += (a[i] - '0') * tens;
        tens *= 10;
    }
    return accum;
}

float float_from_str(const char *a, size_t len) {
    float accum = 0;
    size_t decimal = len/*, exponent = -1*/;
    float tens = 1;

    for (size_t i = 0; i < len; i++) {
        if (a[i] == '.') {
            decimal = i;
        }
        if (a[i] == 'e' || a[i] == 'E') {
            // exponent = i; // TODO : ADD EXPONENT support
        }
    }

    for (size_t i = decimal; i-- > 0;) {
        accum += (a[i] - '0') * tens;
        tens *= 10;
    }

    tens = 0.1;
    for (size_t i = decimal + 1; i < len; i++) {
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


TokenArray tk_tokenize(Arenas *arena, const char *buf, size_t size) {
    TokenArray tokens = { 0 };

    size_t start = 0;
    size_t next = 0;
    size_t col = 0;
    size_t line = 1;
    
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
                /* fall through */
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

        char token[next - start + 1]; // VLA
        memcpy(token, &buf[start], next - start);
        token[next - start] = '\0';

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
                // String ident = { .data = arena_alloc(arena->persistent, sizeof(char) * (next - start + 1)), .len = next - start};
                // memcpy(ident.data, token, next - start);
                // tok.identifier = VIEW_FROM_STRING(ident);
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

    Token tok_eof = (Token) { 0 };
    tok_eof.type = TOKEN_EOF;
    arena_array_append(arena->persistent, tokens, tok_eof);

    return tokens;
}


void tk_print(Token tok) {
    switch (tok.type) {
        case TOKEN_INT_LIT: printf("%u", tok.int_lit); break;
        case TOKEN_FLOAT_LIT: printf("%.2f", tok.float_lit); break;
        case TOKEN_IDENTIFIER: printf("%s", tok.identifier); break;
        default: printf(SV_FMT, SV_ARG(token_info_table[tok.type].name)); break;
    }
    printf("\n");
}

size_t tk_array_sv_list(Arena *a, TokenArray arr, StringView *list[static const 1]) {
    list[0] = arena_alloc(a, sizeof(StringView) * arr.count);

    for (size_t i = 0; i < arr.count; i++) {
        list[0][i] = arr.items[i].lexeme;
    }

    return arr.count;
}

String tk_array_str(Arena *a, TokenArray arr) {
    StringView *list = NULL;
    size_t n = tk_array_sv_list(a, arr, &list);

    return arena_sconcat(a, SV_LITERAL("\n"), n, list);
}


bool tk_is_operator(TokenType type) {
    return (token_info_table[type].flags & TOK_OPERATOR) == TOK_OPERATOR;
}


bool token_has_precedence(TokenType type) {
    return tk_is_operator(type) ||
           (token_info_table[type].flags & TOK_HAS_PREC) == TOK_HAS_PREC;
}


bool tk_is_primitive(TokenType type) {
    return (token_info_table[type].flags & TOK_PRIMITIVE) == TOK_PRIMITIVE;
}


int tk_get_precedence(TokenType type) {
    return token_has_precedence(type) ? token_info_table[type].precedence : -1;
}
