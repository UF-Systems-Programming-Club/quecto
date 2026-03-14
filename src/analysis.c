#include "analysis.h"
#include "error.h"

QuectoType *resolve_binary_type(QuectoType *left, QuectoType *right) {
    return left;
}

QuectoType *analyze_expression(AST *expr, SymbolTable *scope) {
    switch(expr->type) {
        case AST_CALL: {
            SymbolData *signature = get_symbol(scope, expr->callee->ident);

            if (signature->type != SYM_TYPE_PROCEDURE) {
                report_error(0, 0, "Cannot call a non-procedure");
            }
            if (signature->param_count != expr->arg_count) {
                report_error(0, 0, "Call does not match procedure signature"); // probably add line information to AST for better reporting
            }
                        
            return signature->return_types[0];
        }
        case AST_INT_LIT:
            return &default_integer_type;
        case AST_SYMBOL: {
            SymbolData *var = get_symbol(scope, expr->ident);
            return var->qtype;
        }
        case AST_BINARY_OP: {
            QuectoType *left = analyze_expression(expr->left, scope);
            QuectoType *right = analyze_expression(expr->right, scope);

            expr->evaled_type = resolve_binary_type(left, right);
            
            return expr->evaled_type;
        }
        default:
            UNREACHABLE("AST NOT EXPR");
            break;
    }
}

void analyze_statement(AST *statement, SymbolTable *scope) {
    switch (statement->type) {
        case AST_ASSIGNMENT: {
            SymbolData *var = get_symbol(scope, statement->symbol->ident);
            QuectoType *expr_type = analyze_expression(statement->expr, scope);
            
            if (!quecto_types_equal(var->qtype, expr_type)) {
                report_error(statement->line, statement->col, "mismatched types");
            }
            break;
        }
        case AST_DECL: {          
            QuectoType *expr_type = analyze_expression(statement->expr, scope);
            if (statement->qtype->type == QUECTO_UNKNOWN) { // infer from expr
                statement->qtype = expr_type;
                break;
            }
            if (!quecto_types_equal(statement->qtype, expr_type)) {
                report_error(statement->line, statement->col, "declared as ... assigned ..., mismatched types");
                break;
            }
                
            break;
        }
        case AST_IF:
        case AST_WHILE:
        case AST_RETURN:
            break;
        case AST_BLOCK:
            analyze_block(statement, scope);
            break;
        default:
            UNREACHABLE("AST NOT STATEMENT");
            break;
    }
}

void analyze_block(AST *block, SymbolTable *scope) {
    assert(block->type == AST_BLOCK);
    
    for (int i = 0; i < block->count; i++) {
        analyze_statement(block->items[i], scope); // will have to be changed at some point to BLOCK's scope which would be child of PROC's scope
    }
}

void analyze_procedure(AST *procedure) {
    assert(procedure->type == AST_PROCEDURE);
    
    analyze_block(procedure->body, procedure->symbols);
}

void analyze_ast(AST* program) { // assumes ast is a AST_PROGRAM
    assert(program->type == AST_PROGRAM);
    
    for (int i = 0; i < program->count; i++) {
        analyze_procedure(program->items[i]);    
    }
}

