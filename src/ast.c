#include <stdio.h>

#include "ast.h"

void print_ast(AST* ast) {
    switch (ast->type) {
        case AST_FLOAT_LIT:
            printf("%f", ast->float_lit);
            break;
        case AST_INT_LIT:
            printf("%d", ast->int_lit);
            break;
        case AST_DECLARATION:
            printf("%s := ", ast->symbol);
            print_ast(ast->expr);
            break;
        case AST_BINARY_OP:
            printf("(");
            print_ast(ast->left);
            switch (ast->op) {
                case OP_PLUS:
                printf(" + ");
                break;
                case OP_MINUS:
                printf(" - ");
                break;
                case OP_DIVIDE:
                printf(" / ");
                break;
                case OP_MULTIPLY:
                printf(" * ");
                break;
                case OP_ASSIGN:
                printf(" = ");
                break;
            }
            print_ast(ast->right);
            printf(")");
            break;
        case AST_BLOCK:
            printf("{\n");
            for (int i = 0; i < ast->block.count; i++) {
                printf("\t");
                print_ast(ast->block.items[i]);
                printf(",\n");
            }
            printf("}\n");
            break;
        case AST_RETURN:
            printf("return ");
            print_ast(ast->expr);
            break;
        default:
            printf("unknown\n");
            break;
    }
}
