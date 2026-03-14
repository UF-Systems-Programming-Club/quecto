#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "error.h"
#include "symbol_table.h"

void print_ast(AST* ast, int indent) {
    switch (ast->type) {
        case AST_PROGRAM:
            for (int i = 0; i < ast->count; i++) {
                print_ast(ast->items[i], 0);
            }
            break;
        case AST_PROCEDURE:
            print_indent(0, "proc ");
            print_ast(ast->name, 0);

            printf("(");
            for (int i = 0; i < ast->param_count; i++) {
                print_ast(ast->params[i]->symbol, 0);
                printf(": "); print_type(ast->params[i]->qtype);
                if (i < ast->param_count - 1) printf(", ");
            }
            printf(") => (");
            for (int i = 0; i < ast->return_count; i++) {
                print_ast(ast->returns[i]->symbol, 0);
                printf(": "); print_type(ast->returns[i]->qtype);
                if (i < ast->return_count - 1) printf(", ");
            }
            printf(")\n");
            print_ast(ast->body, 0);
            break;
        case AST_CALL:
            print_ast(ast->callee, indent);
            printf("(");
            for (int i = 0; i < ast->arg_count; i++) {
                print_ast(ast->args[i], 0);
                if (i < ast->arg_count - 1) printf(", ");
            }
            printf(")");
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
            print_ast(ast->symbol, indent);
            printf(" = ");
            print_ast(ast->expr, 0);
            printf(";\n");
            break;
        case AST_DECL:
            print_ast(ast->symbol, indent);
            printf(" : ");
            if (ast->qtype != NULL) print_type(ast->qtype);
            printf(" = ");
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
            print_ast(ast->then, indent);
            if (ast->otherwise) print_ast(ast->otherwise, indent);
            break;
        case AST_ELIF:
            print_indent(0, "elif ");
            print_ast(ast->condition, 0);
            printf("\n");
            print_ast(ast->then, indent);
            if (ast->otherwise) print_ast(ast->otherwise, indent);
            break;
        case AST_ELSE:
            print_indent(0, "else\n");
            print_ast(ast->then, indent);
            break;
        case AST_WHILE:
            print_indent(0, "while ");
            print_ast(ast->condition, 0);
            printf("\n");
            print_ast(ast->then, indent);
            break;
        default:
            printf("unknown\n");
            break;
    }
}

