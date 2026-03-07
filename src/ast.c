#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "symbol_table.h"

void print_ast(AST* ast, int indent) {
    switch (ast->type) {
        case AST_PROGRAM:
            for (int i = 0; i < ast->count; i++) {
                print_ast(ast->items[i], 0);
            }
            break;
        case AST_SYMBOL:
            print_indent(0, "%s", ast->ident);
            break;
        case AST_FLOAT_LIT:
            print_indent(0, "%f", ast->float_lit);
            break;
        case AST_INT_LIT:
            print_indent(0, "%d", ast->int_lit);
            break;
        case AST_ASSIGNMENT:
            print_ast(ast->symbol, 0);
            printf(" = ");
            print_ast(ast->expr, 0);
            printf(";\n");
            break;
        case AST_DECL:
            print_ast(ast->symbol, indent);
            printf(" := ");
            print_ast(ast->expr, 0);
            printf(";\n");
            break;
        case AST_BINARY_OP:
            printf("(");
            print_ast(ast->left, 0);
            switch (ast->op) {
                case OP_EQUALS:         printf(" == "); break;
                case OP_LESS_THAN:      printf(" < "); break;
                case OP_GREATER_THAN:   printf(" > "); break;
                case OP_LESS_EQUALS:    printf(" <= "); break;
                case OP_GREATER_EQUALS: printf(" >= "); break;
                case OP_PLUS:           printf(" + "); break;
                case OP_MINUS:          printf(" - "); break;
                case OP_DIVIDE:         printf(" / "); break;
                case OP_MULTIPLY:       printf(" * "); break;
            }
            print_ast(ast->right, 0);
            printf(")");
            break;
        case AST_BLOCK:
            print_indent(0, "{\n");
            for (int i = 0; i < ast->count; i++) {
                print_ast(ast->items[i], indent + 1);
            }
            print_indent(0, "}\n");
            break;
        case AST_RETURN:
            print_indent(0, "return ");
            print_ast(ast->expr, 0);
            printf(";\n");
            break;
        case AST_IF:
            print_indent(0, "if ");
            print_ast(ast->condition, 0);
            printf("\n");
            print_ast(ast->block, indent);
            break;
        default:
            printf("unknown\n");
            break;
    }
}
