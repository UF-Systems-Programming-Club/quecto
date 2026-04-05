#include "analysis.h"
#include "ast.h"
#include "common.h"
#include "error.h"
#include "symbol_table.h"

QuectoType *resolve_binary_type(QuectoType *left, QuectoType *right) {
    if (quecto_types_equal(left, right)) return left;
    if (quecto_is_integer(left) && quecto_is_integer(right)) {
        return quecto_primitive_width[left->type] > quecto_primitive_width[right->type] ? left : right;
    }
    return right;
}

QuectoType *analyze_expression(AnalysisContext *context, AST *expr) {
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
                        
            return signature->return_types[0];
        }
        case AST_INT_LIT:
            return &default_integer_type;
        case AST_SYMBOL: {
            SymbolData *var = get_symbol(context->symbol_table, expr->ident);
            return var->qtype;
        }
        case AST_BINARY_OP: {
            QuectoType *left = analyze_expression(context, expr->left);
            QuectoType *right = analyze_expression(context, expr->right);

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

                return signature->qtype->inner;
            }
        case AST_LIST: {
            QuectoType constructed = (QuectoType) {
                .type = QUECTO_ARRAY,
                .inner = &default_integer_type,
                .array_size = expr->count,
            };

            return arena_intern(context->arena, context->type_intern_table, &constructed, sizeof(QuectoType));
        }
        default:
            UNREACHABLE("AST NOT EXPR");
    }
    return NULL;
}


void analyze_statement(AnalysisContext *context, AST *statement) {
    switch (statement->type) {
        case AST_ASSIGNMENT: {
            SymbolData *var;
            if ((var = get_symbol(context->symbol_table, statement->symbol->ident)) == NULL) {
                report_error(statement->line, statement->col, "assigning to not yet defined symbol");
                break;
            }

            QuectoType *expr_type = analyze_expression(context, statement->expr);
            
            if (!quecto_types_equal(var->qtype, expr_type)) {
                report_error(statement->line, statement->col, "mismatched types");
            }
            break;
        }
        case AST_DECL: {          
            QuectoType *expr_type = analyze_expression(context, statement->expr);
            if (expr_type == NULL) {
                report_error(statement->line, statement->col, "error analyzing expression");
                break;
            }

            SymbolData *symbol = insert_symbol(context->arena, context->symbol_table, statement->symbol->ident, SYM_TYPE_VARIABLE);
            symbol->stack_offset = 0;
            symbol->qtype = statement->qtype;

            if (statement->qtype->type == QUECTO_UNKNOWN) { // infer from expr
                if (expr_type->type == QUECTO_COMP_INT) {
                    statement->qtype = arena_intern(context->arena, context->type_intern_table, &(QuectoType) {.type = QUECTO_U32}, sizeof(QuectoType));
                    symbol->qtype = statement->qtype;
                } else {
                    statement->qtype = expr_type;
                    symbol->qtype = statement->qtype;
                }
                break;
            }
            if (quecto_is_integer(statement->qtype) && expr_type->type == QUECTO_COMP_INT) {
                break;
            } else if (!quecto_types_equal(statement->qtype, expr_type)) {
                report_error(statement->line, statement->col, "declared as ");
                print_type(statement->qtype);
                printf(" but assigned ");
                print_type(expr_type);
                printf(".\n");
                break;
            }
            break;
        }
        case AST_CALL:
        case AST_IF:
        case AST_WHILE:
        case AST_RETURN:
            break;
        case AST_BLOCK:
            analyze_block(context, statement);
            break;
        default:
            UNREACHABLE("AST NOT STATEMENT");
            break;
    }
}

void analyze_block(AnalysisContext *context, AST *block) {
    assert(block->type == AST_BLOCK);

    for (int i = 0; i < block->count; i++) {
        analyze_statement(context, block->items[i]); // will have to be changed at some point to BLOCK's scope which would be child of PROC's scope
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
        signature->param_types[i] = procedure->params[i]->qtype;
    }
    for (int i = 0; i < procedure->return_count; i++) {
        SymbolData *sym = insert_symbol(context->arena, context->symbol_table, procedure->returns[i]->symbol->ident, SYM_TYPE_VARIABLE);
        signature->return_types[i] = procedure->returns[i]->qtype;
    }

    if (procedure->body != NULL)
        analyze_block(context, procedure->body);

    context->symbol_table = procedure->symbols->prev;
}

void analyze_ast(AnalysisContext *context, AST *program) { // assumes ast is a AST_PROGRAM
    assert(program->type == AST_PROGRAM);
    
    for (int i = 0; i < program->count; i++) {
        switch(program->items[i]->type) {
            case AST_PROCEDURE:
                analyze_procedure(context, program->items[i]);    
                break;
            case AST_EXTERN:
                analyze_procedure(context, program->items[i]->externed);
                break;
            default:
                UNREACHABLE("not top-level decl");
                break;
        }
    }
}

