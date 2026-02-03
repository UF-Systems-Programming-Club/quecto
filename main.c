#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

bool is_number(char c) {
    return '0' <= c && c <= '9';
}

typedef enum {
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_NUMBER,
    TOKEN_EOF,

    TOKEN_COUNT // Make sure this token is the last one 
                // listed in the enum, as this is assumed by compile time assertions
} TokenType;

const char *token_to_string_table[] = {
    [TOKEN_PLUS] = "+",
    [TOKEN_MINUS] = "-",
    [TOKEN_MULTIPLY] = "*",
    [TOKEN_DIVIDE] = "/",
    [TOKEN_NUMBER] = "number",
    [TOKEN_EOF] = "end of file"
};

static_assert(sizeof(token_to_string_table) / sizeof(char *) == TOKEN_COUNT,
              "Every token must have a corresponding entry in the token to string table, so add an entry probably");

typedef struct {
    TokenType type;
    union {
        unsigned int number;
    };
} Token;

void print_token(Token tok) {
    switch (tok.type) {
        case TOKEN_PLUS:     printf("+\n"); break;
        case TOKEN_MINUS:    printf("-\n"); break;
        case TOKEN_MULTIPLY: printf("*\n"); break;
        case TOKEN_DIVIDE:   printf("/\n"); break;
        case TOKEN_NUMBER:   printf("%u\n", tok.number); break;
        case TOKEN_EOF:      printf("EOF\n"); break;
    }
}

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenArray;

#define array_append(array, item)\
    do {\
        if (array.count >= array.capacity) {\
            if (array.capacity == 0) array.capacity = 128;\
            array.capacity *= 256;\
            array.items = realloc(array.items, array.capacity * sizeof(*array.items));\
        }\
        array.items[array.count++] = item;\
    } while (0)

typedef struct {
    TokenArray tokens;
    int current;
    bool error;
} ParserState;

typedef enum {
    AST_BINARY_OP,
    AST_NUMBER
} ASTType;

typedef enum {
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE,
} BinaryOp;

typedef struct AST {
    ASTType type;
    union {
        struct {
            BinaryOp op;
            struct AST *left;
            struct AST *right;
        };
        unsigned int number;
    };
} AST;

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

int get_token_precedence(TokenType type) {
    if (token_is_operator(type)) return get_token_precedence_table[type];

    return -1;
}

TokenType peek_next_token(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return TOKEN_EOF;

    return parser->tokens.items[parser->current].type;
}

bool match_next_token(ParserState *parser, TokenType type) {
    if (peek_next_token(parser) == type) {
        return true;
    } else {
        printf("parsing error: expected a \"%s\" but got a \"%s\" instead\n",
                token_to_string_table[type],
                token_to_string_table[peek_next_token(parser)]);
        parser->error = true;
        return false;
    }
}

Token get_next_token(ParserState *parser) {
    if (parser->current >= parser->tokens.count) return parser->tokens.items[parser->tokens.count - 1];;
    return parser->tokens.items[parser->current++];
}

AST *parse_expression(ParserState *parser, int min_prec) {
    if (!match_next_token(parser, TOKEN_NUMBER)) {
        return NULL;
    }

    Token tok = get_next_token(parser);

    AST *left = (AST *)malloc(sizeof(AST));
    left->type = AST_NUMBER;
    left->number = tok.number;

    if (peek_next_token(parser) == TOKEN_EOF) return left;

    if (!token_is_operator(peek_next_token(parser))) {
        printf("parsing error: expected an \"operator\" but got a \"%s\" instead\n",
                token_to_string_table[peek_next_token(parser)]);
        parser->error = true;
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
        }

        op->right = parse_expression(parser, get_token_precedence(tok.type));
        if (parser->error) {
            return NULL;
        }

        left = op;
    }
    
    return left;
};

unsigned int evaluate_ast(AST *ast) {
    if (ast->type == AST_NUMBER) return ast->number;

    unsigned int left = evaluate_ast(ast->left);
    unsigned int right = evaluate_ast(ast->right);

    // TODO: actually check for type here later when more AST types are added
    switch (ast->op) {
        case OP_PLUS:
            return left + right;
        case OP_MINUS:
            return left - right;
        case OP_MULTIPLY:
            return left * right;
        case OP_DIVIDE:
            return left / right;
    }
}

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
    while (start < buf_size) {
        char c = buf[next++];

        Token tok = {0};

        switch (c) {
            case ' ':
            case '\t':
            case '\n':
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

            default:
                if (is_number(c)) {
                    tok.type = TOKEN_NUMBER;
                    tok.number = (unsigned int)(c - '0');
                    array_append(tokens, tok);
                } else {
                    printf("tokenizing error: unrecognized character \"%c\"\n", c);
                }
                break;
        }

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

    AST *ast = parse_expression(&parser, 0);

    if (!parser.error) {
        unsigned int result = evaluate_ast(ast);
        printf("result: %u\n", result);
    }
}
