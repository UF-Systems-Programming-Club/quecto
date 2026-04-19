#include "analysis.h"
#include "ast.h"
#include "common.h"
#include "error.h"
#include "symbol_table.h"

void analyze_ast(AnalysisContext *context, AST *program) { // assumes ast is a AST_PROGRAM
    assert(program->type == AST_PROGRAM);
    
    for (int i = 0; i < program->count; i++) {
        switch(program->items[i]->type) {
            case AST_PROCEDURE: analyze_procedure(context, program->items[i]); break;
            case AST_EXTERN:    analyze_procedure(context, program->items[i]->externed); break;
            default: UNREACHABLE("not top-level decl"); break;
        }
    }
}


void analyze_procedure(AnalysisContext *context, AST *procedure) {
    assert(procedure->type == AST_PROCEDURE);

    SymbolData *signature = insert_symbol(context->arena, context->symbol_table, procedure->name->ident, SYM_TYPE_PROCEDURE);
    
    signature->param_count = procedure->param_count;
    signature->return_count = procedure->return_count;
    signature->local_var_size = 0;

    procedure->symbols = arena_alloc_type(context->arena, SymbolTable);
    procedure->symbols->prev = context->symbol_table;
    context->symbol_table = procedure->symbols;
    
    for (int i = 0; i < procedure->param_count; i++) {
        SymbolData *sym = insert_symbol(context->arena, context->symbol_table, procedure->params[i]->symbol->ident, SYM_TYPE_VARIABLE);
        sym->qtype = procedure->params[i]->evaled_type;
        signature->param_types[i] = procedure->params[i]->evaled_type;
    }
    for (int i = 0; i < procedure->return_count; i++) {
        SymbolData *sym = insert_symbol(context->arena, context->symbol_table, procedure->returns[i]->symbol->ident, SYM_TYPE_VARIABLE);
        sym->qtype = procedure->returns[i]->evaled_type;
        signature->return_types[i] = procedure->returns[i]->evaled_type;
    }

    context->returns = signature->return_types;
    context->return_count = procedure->return_count;

    if (procedure->body != NULL)
        analyze_block(context, procedure->body);

    context->symbol_table = procedure->symbols->prev;
}


void analyze_block(AnalysisContext *context, AST *block) {
    assert(block->type == AST_BLOCK);

    for (int i = 0; i < block->count; i++) {
        analyze_statement(context, block->items[i]);
    }
}


void analyze_statement(AnalysisContext *context, AST *statement) {
    switch (statement->type) {
        case AST_ASSIGNMENT: analyze_assignment(context, statement); break;
        case AST_DECL: analyze_declaration(context, statement); break;
        case AST_BLOCK: analyze_block(context, statement); break;
        case AST_CALL: analyze_expression(context, statement, NULL); break;
        case AST_WHILE:
        case AST_IF:
        case AST_ELIF: analyze_expression(context, statement->condition, NULL);
        case AST_ELSE: analyze_statement(context, statement->then);
            if (statement->otherwise) // this should always be NULL for ELSE and WHILE
                analyze_statement(context, statement->otherwise);
            break;
        case AST_RETURN: 
            if (statement->expr)
                analyze_expression(context, statement->expr, context->returns[0]);
            break;
        default:
            UNREACHABLE("AST NOT STATEMENT");
            break;
    }
}


void analyze_declaration(AnalysisContext *context, AST *decl) {
    SymbolData *symbol = insert_symbol(context->arena, context->symbol_table, decl->symbol->ident, SYM_TYPE_VARIABLE);
    symbol->stack_offset = 0;

    if (decl->expr == NULL && decl->evaled_type->type == QUECTO_UNKNOWN) {
        report_error(decl->line, decl->col, "cannot infer type without expr");
        return;
    }

    QuectoType *is = analyze_expression(context, decl->expr, decl->evaled_type);
    if (decl->evaled_type->type == QUECTO_UNKNOWN) {
        decl->evaled_type = (is->type == QUECTO_COMP_INT) ? &default_integer_type : is;
    }

    if (!quecto_types_equal(decl->evaled_type, is)) {
        report_error(decl->line, decl->col, "declared as ");  print_type(decl->evaled_type); printf(" but assigned "); print_type(is); printf(".\n");
    }

    symbol->qtype = decl->evaled_type;
}


void analyze_assignment(AnalysisContext *context, AST *assignment) {
    SymbolData *var;
    if ((var = get_symbol(context->symbol_table, assignment->symbol->ident)) == NULL) {
        report_error(assignment->line, assignment->col, "assigning to not yet defined symbol");
        return;
    }

    QuectoType *expr_type = analyze_expression(context, assignment->expr, var->qtype);

    if (!quecto_types_equal(var->qtype, expr_type)) {
        report_error(assignment->line, assignment->col, "mismatched types");
    }
}


QuectoType *analyze_expression(AnalysisContext *context, AST *expr, QuectoType *expected) {
    switch(expr->type) {
        case AST_CALL: {
             SymbolData *signature;
            if ((signature = get_symbol(context->symbol_table, expr->callee->ident)) == NULL) { // for now a single analysis pass, in future pass for decls and then pass for valid code
                report_error(expr->line, expr->col, "cannot call undefined procedure");
                break;
            }

            if (signature->type != SYM_TYPE_PROCEDURE) {
                report_error(0, 0, "Cannot call a non-procedure");
            }
            if (signature->param_count != expr->arg_count) {
                report_error(0, 0, "Call does not match procedure signature"); // probably add line information to AST for better reporting
            }

            for (int i = 0; i < expr->arg_count; i++) {
                analyze_expression(context, expr->args[i], signature->param_types[i]);
            }

            expr->evaled_type = signature->return_types[0];
            return signature->return_types[0];
        }
        case AST_INT_LIT:
            expr->evaled_type = expected;
            if (expected == NULL || expected->type == QUECTO_UNKNOWN) {
                expr->evaled_type = &default_integer_type;
            }
            return expr->evaled_type;
        case AST_SYMBOL: {
            SymbolData *var = get_symbol(context->symbol_table, expr->ident);

            expr->evaled_type = var->qtype;
            return var->qtype;
        }
        case AST_BINARY_OP: {
            QuectoType *left = analyze_expression(context, expr->left, expected);
            QuectoType *right = analyze_expression(context, expr->right, expected);

            bool left_valid = left != NULL && left->type != QUECTO_COMP_INT;
            bool right_valid = right != NULL && right->type != QUECTO_COMP_INT;

            if (!left_valid && right_valid) left = analyze_expression(context, expr->left, right);
            if (!right_valid && left_valid) right = analyze_expression(context, expr->right, left);

            expr->evaled_type = resolve_binary_type(left, right);
            return expr->evaled_type;
        }
        case AST_INDEX: {
            SymbolData *signature = get_symbol(context->symbol_table, expr->access->ident);
            if (signature == NULL) {
                report_error(0, 0, "undefined variable");
            }

            if (signature->type != SYM_TYPE_VARIABLE) {
                report_error(0, 0, "cannot indexa non variable");
            }

            if (signature->qtype->type != QUECTO_ARRAY) {
                report_error(0, 0, "cannot index a non array");
            }

            analyze_expression(context, expr->index, NULL);

            expr->evaled_type = signature->qtype->inner;
            return signature->qtype->inner;
        }
        case AST_LIST: {
            for (int i = 0; i < expr->count; i++) {
                analyze_expression(context, expr->items[i], expected->inner);
            }
                
            expr->evaled_type = expected;
            return expr->evaled_type;
            }
        default:
            UNREACHABLE("AST NOT EXPR");
    }
    return NULL;
}


QuectoType *resolve_binary_type(QuectoType *left, QuectoType *right) {
    if (quecto_types_equal(left, right)) return left;
    if (quecto_is_integer(left) && quecto_is_integer(right)) {
        return quecto_primitive_width[left->type] > quecto_primitive_width[right->type] ? left : right;
    }
    return right;
}
