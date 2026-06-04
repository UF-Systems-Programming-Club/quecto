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

QuectoType *pointer_of_type(AnalysisContext *context, QuectoType *type) {
    QuectoType pointer = { 0 };
    pointer.type = QUECTO_POINTER;
    pointer.inner = type;
    return arena_intern(context->arena, context->type_intern_table, &pointer, sizeof(QuectoType));
}


SymbolData *lookup_or_error(AnalysisContext *context, AST *symbol, const char *message) {
    SymbolData *var = get_symbol(context->symbol_table, symbol->ident);
    return var ? var : (report_error(symbol->line, symbol->col, message), NULL);
}


void analyze_ast(AnalysisContext *context, AST *program) { // assumes ast is a AST_PROGRAM
    assert(program->type == AST_PROGRAM);
    
    for (int i = 0; i < program->count; i++) {
        switch(program->items[i]->type) {
            case AST_PROCEDURE: analyze_procedure(context, program->items[i], false); break;
            case AST_EXTERN:    analyze_procedure(context, program->items[i]->externed, true); break;
            default: UNREACHABLE("not top-level decl"); break;
        }
    }
}


void analyze_procedure(AnalysisContext *context, AST *procedure, bool externed) {
    assert(procedure->type == AST_PROCEDURE);

    SymbolData *symbol = insert_symbol(context->arena, context->symbol_table, procedure->name->ident);

    procedure->symbols = arena_alloc_type(context->arena, SymbolTable);
    procedure->symbols->prev = context->symbol_table;
    context->symbol_table = procedure->symbols;

    QuectoType qtype = { 0 };
    qtype.type = QUECTO_PROCEDURE;

    ProcSignature signature = { 0 };

    signature.param_count = procedure->param_count;
    for (int i = 0; i < procedure->param_count; i++) {
        SymbolData *sym = insert_symbol(context->arena, context->symbol_table, procedure->params[i]->lhs->ident);
        sym->qtype = procedure->params[i]->evaled_type;
        signature.param_types[i] = procedure->params[i]->evaled_type;
    }

    signature.return_count = procedure->return_count;
    for (int i = 0; i < procedure->return_count; i++) {
        SymbolData *sym = insert_symbol(context->arena, context->symbol_table, procedure->returns[i]->lhs->ident);
        sym->qtype = procedure->returns[i]->evaled_type;
        signature.return_types[i] = procedure->returns[i]->evaled_type;
    }

    qtype.signature = arena_intern(context->arena, context->type_intern_table, &signature, sizeof(signature));

    symbol->qtype = arena_intern(context->arena, context->type_intern_table, &qtype, sizeof(QuectoType));
    symbol->externed = externed;
    
    context->current_procedure = symbol->qtype->signature;

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
            if (statement->rhs)
                analyze_expression(context, statement->rhs, context->current_procedure->return_types[0]);
            break;
        default:
            UNREACHABLE("AST NOT STATEMENT");
            break;
    }
}


void analyze_declaration(AnalysisContext *context, AST *decl) {
    assert(decl->type == AST_DECL);
    
    SymbolData *symbol = insert_symbol(context->arena, context->symbol_table, decl->lhs->ident);

    if (decl->rhs == NULL && decl->evaled_type->type == QUECTO_UNKNOWN) {
        report_error(decl->line, decl->col, "cannot infer type without expr");
        return;
    }

    QuectoType *is = analyze_expression(context, decl->rhs, decl->evaled_type);
    if (decl->evaled_type->type == QUECTO_UNKNOWN) {
        decl->evaled_type = (is->type == QUECTO_COMP_INT) ? &default_integer_type : is;
    }

    if (!quecto_types_equal(decl->evaled_type, is)) {
        //report_error(decl->line, decl->col, "declared as "); // todo: create string/char buffer builder to ease printing for multiple destinations (i.e file output, errors, regular)
        printf("declared as "); print_type(decl->evaled_type); printf(" but assigned "); print_type(is); printf(".\n");
    }

    symbol->qtype = decl->evaled_type;
}




void analyze_assignment(AnalysisContext *context, AST *assignment) {
    SymbolData *var;
    QuectoType *expr_type;
    switch (assignment->lhs->type) { // should rename this b/c not just symbol anymore
        case AST_SYMBOL:
            var = lookup_or_error(context, assignment->lhs, "assignment to undefined symbol");
            expr_type = analyze_expression(context, assignment->rhs, var->qtype);
            if (!quecto_types_equal(var->qtype, expr_type)) {
                report_error(assignment->line, assignment->col, "mismatched types");
            }
            break;
        case AST_INDEX:
            var = lookup_or_error(context, get_underlying_symbol_from(assignment->lhs), "assignment to undefined symbol");
            analyze_index(context, assignment->lhs);
            expr_type = analyze_expression(context, assignment->rhs, var->qtype->inner);
            if (!quecto_types_equal(var->qtype->inner, expr_type)) {
                report_error(assignment->line, assignment->col, "mismatched types");
            }
            break;
        default: {
            report_error(assignment->line, assignment->col, "expr not assignable");
        }
    }

}


QuectoType *analyze_expression(AnalysisContext *context, AST *expr, QuectoType *expected) {
    switch(expr->type) {
        case AST_CALL: return analyze_call(context, expr);
        case AST_INT_LIT: return expr->evaled_type = (expected == NULL || expected->type == QUECTO_UNKNOWN) ? &default_integer_type : expected; 
        case AST_SYMBOL: return analyze_symbol(context, expr);
        case AST_BINARY_OP: return analyze_binary_op(context, expr, expected);
        case AST_INDEX: return analyze_index(context, expr);
        case AST_LIST: return analyze_list(context, expr, expected);
        case AST_REF: return analyze_ref(context, expr);
        default: UNREACHABLE("AST NOT EXPR");
    }
    return NULL;
}


QuectoType *analyze_call(AnalysisContext *context, AST *call) {
    SymbolData *symbol = lookup_or_error(context, call->callee, "cannot call an undefined procedure");

    if (!quecto_is_procedure(symbol->qtype)) {
        report_error(call->line, call->col, "Cannot call a non-procedure");
        return NULL;
    }

    ProcSignature *call_signature = symbol->qtype->signature;

    if (call_signature->param_count != call->arg_count) {
        report_error(call->line, call->col, "Call does not match procedure signature");
        return NULL;
    }

    for (int i = 0; i < call->arg_count; i++) {
        analyze_expression(context, call->args[i], call_signature->param_types[i]);
    }

    return call->evaled_type = call_signature->return_types[0];
}


QuectoType *analyze_index(AnalysisContext *context, AST *index) {

    // if (!quecto_is_array(signature->qtype)) {
    //     report_error(index->line, index->col, "cannot index a non array");
    //     return NULL;
    // }
    index->base->evaled_type = analyze_expression(context, index->base, NULL);
    index->index->evaled_type = analyze_expression(context, index->index, NULL);
    return index->evaled_type = index->base->evaled_type->inner;
}

QuectoType *analyze_ref(AnalysisContext *context, AST *ref) {
    QuectoType *inner = analyze_expression(context, ref->base, NULL);
    // todo: verify that the reference is valid i.e not a constant integer etc
    return ref->evaled_type = pointer_of_type(context, inner);
};

QuectoType *analyze_binary_op(AnalysisContext *context, AST *op, QuectoType *expected) {
    QuectoType *left = analyze_expression(context, op->left, expected);
    QuectoType *right = analyze_expression(context, op->right, expected);

    bool left_valid = left != NULL && left->type != QUECTO_COMP_INT;
    bool right_valid = right != NULL && right->type != QUECTO_COMP_INT;

    if (!left_valid && right_valid) left = analyze_expression(context, op->left, right);
    if (!right_valid && left_valid) right = analyze_expression(context, op->right, left);

    return op->evaled_type = resolve_binary_type(left, right);
}


QuectoType *analyze_list(AnalysisContext *context, AST *list, QuectoType *expected) {
    if (expected == NULL || expected->type == QUECTO_UNKNOWN) {
        report_error(list->line, list->col, "list must be specified with type.");
        return NULL;
    }
    
    for (int i = 0; i < list->count; i++) {
        analyze_expression(context, list->items[i], expected->inner);
    }
        
    return  list->evaled_type = expected;
}


QuectoType *analyze_symbol(AnalysisContext *context, AST *symbol) {
    SymbolData *var = lookup_or_error(context, symbol, "undefined variable");
    return var ? (symbol->evaled_type = var->qtype) : NULL;
}


QuectoType *analyze_access(AnalysisContext *context, AST *access) {
    return NULL;
}


