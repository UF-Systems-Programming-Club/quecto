#include "AST.h"
#include <stdio.h>

AST evaluate_ast(AST *ast) {
    switch (ast->type) {
        case AST_BINARY_OP:
            {
            AST ret = {0};
            AST left = evaluate_ast(ast->left);
            AST right = evaluate_ast(ast->right);

            ret.type = (left.type == AST_INT_LIT && right.type == AST_FLOAT_LIT || left.type == AST_FLOAT_LIT && right.type) ? AST_FLOAT_LIT : AST_INT_LIT; // promotion

            switch (ret.type) {
                case AST_INT_LIT:
                switch (ast->op) {
                    case OP_PLUS:
                        ret.int_lit = (unsigned int) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) + (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                    case OP_MINUS:
                        ret.int_lit = (unsigned int) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) - (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                    case OP_MULTIPLY:
                        ret.int_lit = (unsigned int) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) * (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                    case OP_DIVIDE:
                        ret.int_lit = (unsigned int) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) / (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                }
                break;
                case AST_FLOAT_LIT:
                switch (ast->op) {
                    case OP_PLUS:
                        ret.float_lit = (float) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) + (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                    case OP_MINUS:
                        ret.float_lit = (float) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) - (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                    case OP_MULTIPLY:
                        ret.float_lit = (float) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) * (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                    case OP_DIVIDE:
                        ret.float_lit = (float) ((left.type == AST_FLOAT_LIT ? left.float_lit : left.int_lit) / (right.type == AST_FLOAT_LIT ? right.float_lit : right.int_lit));
                        break;
                }
                break;
                }
                return ret;
            }
        case AST_FLOAT_LIT:
        case AST_INT_LIT:
            return (*ast);
        default:
            return (AST){0};
    }
}

void print_ast(AST* ast) {
    switch (ast->type) {
        case AST_FLOAT_LIT:
            printf("%f", ast->float_lit);
            break;
        case AST_INT_LIT:
            printf("%d", ast->int_lit);
            break;
        case AST_DECLARATION:
            printf("declaration: {symbol} {expr}\n");
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

            printf("{ \"block\": [\n");
            for (int i = 0; i < ast->block.count; i++) {
                printf("\t");
                print_ast(ast->block.items[i]);
                printf(",\n");
            }
            printf("]}\n");
            break;
        case AST_RETURN:
            printf("return");
            break;
        default:
            printf("unknown\n");
            break;
    }
}
