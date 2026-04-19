#include <stdio.h>

#include "parser.h"
#include "ast.h"
#include "common.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "error.h"

int get_token_precedence_table[] = {
    [TOKEN_EQUALS_EQUALS] = 1,
    [TOKEN_GREATER_EQUALS] = 1,
    [TOKEN_LESS_EQUALS] = 1,
    [TOKEN_LESS_THAN] = 1,
    [TOKEN_GREATER_THAN] = 1,
    [TOKEN_PLUS] = 2,
    [TOKEN_MINUS] = 2,
    [TOKEN_MULTIPLY] = 3,
    [TOKEN_DIVIDE] = 3,
    [TOKEN_OPEN_PAREN] = 4,
    [TOKEN_OPEN_SQUARE] = 4,
    [TOKEN_PERIOD] = 5,
};

bool token_is_operator(TokenType type) {
    return (TOKEN_PLUS <= type && type <= TOKEN_GREATER_THAN) || (type == TOKEN_OPEN_PAREN) || (type == TOKEN_OPEN_SQUARE) || (type == TOKEN_PERIOD);
}


bool token_is_primitive(TokenType type) {
    return TOKEN_U8 <= type && type <= TOKEN_I32;
}


int get_token_type_precedence(TokenType type) {
    return (token_is_operator(type)) ? get_token_precedence_table[type] : -1;
}


Token *get_next_token(ParserState *parser) {
    return (parser->current < parser->tokens.count) ? &parser->tokens.items[parser->current++] :
            &parser->tokens.items[parser->tokens.count - 1];
}


TokenType peek_token_type(ParserState *parser, int dist) {
    return (parser->current + dist < parser->tokens.count) ? parser->tokens.items[parser->current + dist].type :
                 parser->tokens.items[parser->tokens.count - 1].type;
}


bool match_next_token(ParserState *parser, Token *tok, TokenType type) {
    if (peek_token_type(parser, 0) == type) {
        *tok = *get_next_token(parser);
        return true;
    }
    return false;
}


bool expect_next_token(ParserState *parser, Token *tok, TokenType type) {
    if (match_next_token(parser, tok, type)) {
        return true;
    } else {
        tok = get_next_token(parser);
        report_error(tok->line, tok->col, "expected \'%s\' but got \'"sv_fmt"\' instead\n",
                     token_to_string_table[type],
                     sv_arg(tok->lexeme));
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

    tok = get_next_token(parser);
    report_error_without_exit(tok->line, tok->col, "expected ");
    for (int i = 0; i < count - 1; i++) {
        printf("%s, ", token_to_string_table[va_arg(args, TokenType)]);
    }
    printf("or %s, ", token_to_string_table[va_arg(args, TokenType)]);
    printf("but got %s instead\n", token_to_string_table[peek_token_type(parser, -1)]);

    va_end(args);

    exit(0);
    return false;
}


AST *parse_identifier(ParserState *parser) {
    Token tok;
    expect_next_token(parser, &tok, TOKEN_IDENTIFIER);

    AST *symbol = arena_alloc_type(parser->arena, AST);
    symbol->type = AST_SYMBOL;
    symbol->ident = tok.identifier;
    return symbol;
}


QuectoType *parse_type(ParserState *parser) {
    QuectoType qtype = { 0 };

    Token *tok = get_next_token(parser);
    switch(tok->type) {
        case TOKEN_I32:     qtype.type = QUECTO_I32; break;
        case TOKEN_U32:     qtype.type = QUECTO_U32; break;
        case TOKEN_I8:      qtype.type = QUECTO_I8; break;
        case TOKEN_U8:      qtype.type = QUECTO_U8; break;                    
        default: {
            // report_error(tok->line, tok->col, "not a recognized type");
            parser->current--;
            qtype.type = QUECTO_UNKNOWN;
        }
    }

    QuectoType *Q = arena_intern(parser->arena, parser->type_intern_table, &qtype, sizeof(QuectoType));

    if (peek_token_type(parser, 0) == TOKEN_OPEN_SQUARE) {
        expect_next_token(parser, tok, TOKEN_OPEN_SQUARE);
        expect_next_token(parser, tok, TOKEN_INT_LIT);
        
        QuectoType qtype_outer;
        qtype_outer.type = QUECTO_ARRAY;
        qtype_outer.inner = Q;
        qtype_outer.array_size = tok->int_lit;
        
        expect_next_token(parser, tok, TOKEN_CLOSE_SQUARE);
        return arena_intern(parser->arena, parser->type_intern_table, &qtype_outer, sizeof(QuectoType));
    }
    
    return Q;
}


AST *parse_program(ParserState *parser) {
    AST *program = arena_alloc_type(parser->arena, AST);
    program->type = AST_PROGRAM;

    while (peek_token_type(parser, 0) != TOKEN_EOF) {
        AST *statement;
        if (peek_token_type(parser, 0) == TOKEN_EXTERN) statement = parse_extern(parser);
        else statement = parse_procedure(parser);
        arena_array_append(parser->arena, *program, statement);
    }

    return program;
}


AST *parse_extern(ParserState *parser) {
    Token tok;

    AST *exter = arena_alloc_type(parser->arena, AST);
    exter->type = AST_EXTERN;
    expect_next_token(parser, &tok, TOKEN_EXTERN);
    exter->externed = parse_procedure(parser);

    return exter;    
}


AST *parse_procedure(ParserState *parser) {
    Token tok;

    AST *procedure = arena_alloc_type(parser->arena, AST);
    procedure->type = AST_PROCEDURE;

    expect_next_token(parser, &tok, TOKEN_PROC);
    
    procedure->line = tok.line;
    procedure->col = tok.col;
    
    expect_next_token(parser, &tok, TOKEN_IDENTIFIER);

    procedure->name = arena_alloc_type(parser->arena, AST);
    procedure->name->type = AST_SYMBOL;
    procedure->name->ident = tok.identifier;

    procedure->param_count = parse_params(parser, procedure->params);
    expect_next_token(parser, &tok, TOKEN_ARROW);

    procedure->return_count = parse_returns(parser, procedure->returns);

    assert(procedure->return_count <= 1 && "no support for multiple return values yet");

    if (match_next_token(parser, &tok, TOKEN_OPEN_CURLY)) {
        procedure->body = arena_alloc_type(parser->arena, AST);
        procedure->body->type = AST_BLOCK;

        while (!match_next_token(parser, &tok, TOKEN_CLOSE_CURLY)) {
            AST *s = parse_statement(parser);
            arena_array_append(parser->arena, *procedure->body, s);
        }
    }

    return procedure;
}

ParserClsfcn classify_statement(ParserState *parser) {
    switch(peek_token_type(parser, 0)) {
        case TOKEN_OPEN_CURLY: return PCLF_BLOCK;
        case TOKEN_IDENTIFIER: {
            switch(peek_token_type(parser, 1)) {
                case TOKEN_COLON: return PCLF_DECL;
                case TOKEN_EQUALS: return PCLF_ASSIGN;
                default: break;
            }   
        }
        break;
        case TOKEN_RETURN: return PCLF_RET;
        case TOKEN_IF: return PCLF_IF;
        case TOKEN_WHILE: return PCLF_WHILE;
        default: break;
    }
    return PCLF_EXPR;
}


AST *parse_block(ParserState *parser) {
    AST *block = arena_alloc_type(parser->arena, AST);
    block->type = AST_BLOCK;

    Token tok;
    expect_next_token(parser, &tok, TOKEN_OPEN_CURLY);
    while (!match_next_token(parser, &tok, TOKEN_CLOSE_CURLY)) {
        AST *s = parse_statement(parser);
        arena_array_append(parser->arena, *block, s);
    }

    return block;
}


AST *parse_if(ParserState *parser) {
    AST *_if = arena_alloc_type(parser->arena, AST);
    _if->type = AST_IF;

    Token tok;
    expect_next_token(parser, &tok, TOKEN_IF);

    _if->condition = parse_expression(parser, 0);
    _if->then = parse_statement(parser);
    _if->otherwise = parse_if_chain(parser);

    return _if;
}


AST *parse_if_chain(ParserState *parser) {
    Token tok;
    if (match_next_token(parser, &tok, TOKEN_ELIF)) {
        AST *statement = arena_alloc_type(parser->arena, AST);

        statement->type = AST_ELIF;
        statement->condition = parse_expression(parser, 0);
        statement->then = parse_statement(parser);
        statement->otherwise = parse_if_chain(parser);

        return statement;
    } else if (match_next_token(parser, &tok, TOKEN_ELSE)) {
        AST *statement = arena_alloc_type(parser->arena, AST);

        statement->type = AST_ELSE;
        statement->condition = NULL;
        statement->then = parse_statement(parser);
        statement->otherwise = NULL;

        return statement;
    }

    return NULL;
}


AST *parse_while(ParserState *parser) {
    AST *_while = arena_alloc_type(parser->arena, AST);
    _while->type = AST_WHILE;

    Token tok;
    expect_next_token(parser, &tok, TOKEN_WHILE);

    _while->condition = parse_expression(parser, 0);
    _while->then = parse_statement(parser);

    return _while;
}


AST *parse_assignment(ParserState *parser) {
    AST *assignment = arena_alloc_type(parser->arena, AST);
    assignment->type = AST_ASSIGNMENT;

    Token tok;

    assignment->symbol = parse_identifier(parser);
    expect_next_token(parser, &tok, TOKEN_EQUALS);
    assignment->expr = parse_expression(parser, 0);

    return assignment;
}


AST *parse_return(ParserState *parser) {
    AST *_return = arena_alloc_type(parser->arena, AST);
    _return->type = AST_RETURN;

    Token tok;
    
    expect_next_token(parser, &tok, TOKEN_RETURN);
    _return->expr = parse_expression(parser, 0);

    return _return;
}


AST *parse_stmt_declaration(ParserState *parser) {
    AST *decl = arena_alloc_type(parser->arena, AST);
    decl->type = AST_DECL;

    Token tok;

    decl->symbol = parse_identifier(parser);
    expect_next_token(parser, &tok, TOKEN_COLON);
    decl->evaled_type = parse_type(parser);

    if (match_next_token(parser, &tok, TOKEN_EQUALS)) {
    decl->expr = (peek_token_type(parser, 0) == TOKEN_OPEN_CURLY) ?
                parse_braced_initializer(parser) : parse_expression(parser, 0);
    };

    return decl;
}


AST *parse_statement(ParserState *parser) {
    Token tok;

    AST *statement = NULL;

    switch (classify_statement(parser)) {
        case PCLF_BLOCK:  return parse_block(parser);
        case PCLF_IF:     return parse_if(parser); 
        case PCLF_WHILE:  return parse_while(parser);
        case PCLF_ASSIGN: statement = parse_assignment(parser); break;
        case PCLF_DECL:   statement = parse_stmt_declaration(parser); break;
        case PCLF_RET:    statement = parse_return(parser); break;
        case PCLF_EXPR:   statement = parse_expression(parser, 0); break;
        default: UNREACHABLE("invalid");
    }

    expect_next_token(parser, &tok, TOKEN_SEMICOLON);
    return statement;
}


AST *parse_braced_initializer(ParserState *parser) {
    Token tok;
    AST *ast = arena_alloc_type(parser->arena, AST);
    expect_next_token(parser, &tok, TOKEN_OPEN_CURLY);
    ast->type = AST_LIST;
    while (!match_next_token(parser, &tok, TOKEN_CLOSE_CURLY)) {
        AST *s = parse_expression(parser, 0);
        if (peek_token_type(parser, 0) != TOKEN_CLOSE_CURLY) expect_next_token(parser, &tok, TOKEN_COMMA);
        arena_array_append(parser->arena, *ast, s);
    }
    return ast;
}


AST *parse_binary_op(ParserState *parser, AST *left) {
    AST *op = arena_alloc_type(parser->arena, AST);
    op->type = AST_BINARY_OP;
    op->left = left;
    
    Token tok;
    tok = *get_next_token(parser);
    switch (tok.type) {
        case TOKEN_EQUALS_EQUALS:   op->op = OP_EQUALS; break;
        case TOKEN_LESS_THAN:       op->op = OP_LESS_THAN; break;
        case TOKEN_GREATER_THAN:    op->op = OP_GREATER_THAN; break;
        case TOKEN_LESS_EQUALS:     op->op = OP_LESS_EQUALS; break;
        case TOKEN_GREATER_EQUALS:  op->op = OP_GREATER_EQUALS; break;
        case TOKEN_PLUS:            op->op = OP_PLUS;     break;
        case TOKEN_MINUS:           op->op = OP_MINUS;    break;
        case TOKEN_MULTIPLY:        op->op = OP_MULTIPLY; break;
        case TOKEN_DIVIDE:          op->op = OP_DIVIDE;   break;
        default:                    UNREACHABLE("TokenType");
    }

    op->right = parse_expression(parser, get_token_type_precedence(tok.type));

    return op;
}


AST *parse_call(ParserState *parser, AST *on) {
    AST *call = arena_alloc_type(parser->arena, AST);
    call->type = AST_CALL;
    call->callee = on;

    Token tok;
    expect_next_token(parser, &tok, TOKEN_OPEN_PAREN);
    
    call->arg_count = parse_args(parser, call->args);    
    return call;
}


AST *parse_index(ParserState *parser, AST *on) {
    AST *index = arena_alloc_type(parser->arena, AST);
    index->type = AST_INDEX;
    index->access = on;

    Token tok;
    expect_next_token(parser, &tok, TOKEN_OPEN_SQUARE);
    index->index = parse_expression(parser, 0);
    expect_next_token(parser, &tok, TOKEN_CLOSE_SQUARE);

    return index;
}


AST *parse_access(ParserState *parser, AST *on) {
    AST *access = arena_alloc_type(parser->arena, AST);
    access->type = AST_ACCESS;
    access->on = on;

    Token tok;
    expect_next_token(parser, &tok, TOKEN_PERIOD);
    access->what = parse_identifier(parser);
    
    return access;
}


AST *parse_expression(ParserState *parser, int min_prec) {  
    Token tok;
    expect_next_token_multiple(parser, &tok, 4, TOKEN_INT_LIT, TOKEN_FLOAT_LIT, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN);

    AST *left;

    switch (tok.type) {
        case TOKEN_INT_LIT:
            left = arena_alloc_type(parser->arena, AST);
            left->type = AST_INT_LIT;
            left->int_lit = tok.int_lit;
            break;
        case TOKEN_FLOAT_LIT:
            left = arena_alloc_type(parser->arena, AST);
            left->type = AST_FLOAT_LIT;
            left->float_lit = tok.float_lit;
            break;
        case TOKEN_IDENTIFIER:
            left = arena_alloc_type(parser->arena, AST);
            left->type = AST_SYMBOL;
            left->ident = tok.identifier;
            break;
        case TOKEN_OPEN_PAREN:
            left = parse_expression(parser, 0);
            expect_next_token(parser, &tok, TOKEN_CLOSE_PAREN);
            break;
        default:
            UNREACHABLE("TokenType");
            break;
    }

    // NOTE: utilizes the fact that get precedence returns -1 when the next token is
    // not an operator
    while (get_token_type_precedence(peek_token_type(parser, 0)) > min_prec) {
        switch (peek_token_type(parser, 0)) {
            case TOKEN_OPEN_SQUARE: left = parse_index(parser, left); break;
            case TOKEN_OPEN_PAREN:  left = parse_call(parser, left); break;
            case TOKEN_PERIOD:      left = parse_access(parser, left); break;
            default:                left = parse_binary_op(parser, left); break;
        }
    }

    return left;
}


AST *parse_param_declaration(ParserState *parser) {
    Token tok;

    AST *decl = arena_alloc_type(parser->arena, AST);
    decl->type = AST_DECL;

    decl->symbol = parse_identifier(parser);
    expect_next_token(parser, &tok, TOKEN_COLON);
    decl->evaled_type = parse_type(parser);

    return decl;
}


size_t parse_args(ParserState *parser, AST *list[MAX_PARAMS]) {
    Token tok;

    size_t count = 0;
    while (!match_next_token(parser, &tok, TOKEN_CLOSE_PAREN) && count < MAX_PARAMS) {
        list[count++] = parse_expression(parser, 0);
        if (peek_token_type(parser, 0) != TOKEN_CLOSE_PAREN) expect_next_token(parser, &tok, TOKEN_COMMA);
    }

    assert(count < MAX_PARAMS && "arg list excedes limit");

    return count;
}


size_t parse_params(ParserState *parser, AST *list[MAX_PARAMS]) {
    Token tok;

    expect_next_token(parser, &tok, TOKEN_OPEN_PAREN);
    size_t count = 0;
    while (!match_next_token(parser, &tok, TOKEN_CLOSE_PAREN) && count < MAX_PARAMS) {
        list[count++] = parse_param_declaration(parser);
        if (peek_token_type(parser, 0) != TOKEN_CLOSE_PAREN) expect_next_token(parser, &tok, TOKEN_COMMA);
    }

    assert(count < MAX_PARAMS && "parameter list excedes limit");

    return count;
}


size_t parse_returns(ParserState *parser, AST *list[MAX_PARAMS]) {
    Token tok;

    expect_next_token(parser, &tok, TOKEN_OPEN_PAREN);
    size_t count = 0;
    while (!match_next_token(parser, &tok, TOKEN_CLOSE_PAREN) && count < MAX_PARAMS) {
        list[count++] = parse_param_declaration(parser);
        if (peek_token_type(parser, 0) != TOKEN_CLOSE_PAREN) expect_next_token(parser, &tok, TOKEN_COMMA);
    }

    assert(count < MAX_PARAMS && "parameter list excedes limit");

    return count;
}


