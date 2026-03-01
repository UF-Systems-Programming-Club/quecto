#include <stdio.h>

#include "ast.h"
#include "symbol_table.h"

void print_ast(AST* ast, int indent) {
    switch (ast->type) {
        case AST_PROGRAM:
            for (int i = 0; i < ast->count; i++) {
                print_ast(ast->items[i], 0);
            }
            break;
        case AST_SYMBOL:
            printf("%s", ast->ident);
            break;
        case AST_FLOAT_LIT:
            printf("%f", ast->float_lit);
            break;
        case AST_INT_LIT:
            printf("%d", ast->int_lit);
            break;
        case AST_ASSIGNMENT:
            print_ast(ast->symbol, 0);
            print_indent(0, " = ");
            print_ast(ast->expr, 0);
            printf(";\n");
            break;
        case AST_DECL:
            print_ast(ast->symbol, 0);
            print_indent(0, " := ");
            print_ast(ast->expr, indent + 1);
            printf(";\n");
            break;
        case AST_BINARY_OP:
            printf("(");
            print_ast(ast->left, 0);
            switch (ast->op) {
                case OP_PLUS:     printf(" + "); break;
                case OP_MINUS:    printf(" - "); break;
                case OP_DIVIDE:   printf(" / "); break;
                case OP_MULTIPLY: printf(" * "); break;
            }
            print_ast(ast->right, 0);
            printf(")");
            break;
        case AST_BLOCK:
            print_indent(0, "{\n");
            print_indent(1, "symbols = [\n");
            // print_symbol_table(ast->block.symbol_table, indent + 1);
            print_indent(1, "],\n")
            print_indent(1, "block = [\n");
            for (int i = 0; i < ast->count; i++) {
                print_ast(ast->items[i], indent + 2);
                printf(",\n");
            }
            print_indent(1, "]\n");
            print_indent(0, "}");
            break;
        case AST_RETURN:
            print_indent(0, "return ");
            print_ast(ast->expr, 0);
            printf(";");
            break;
        default:
            printf("unknown\n");
            break;
    }
}
