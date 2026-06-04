#include "emission.h"
#include "cfg.h"
#include "common.h"
#include "ir.h"
#include "symbol_table.h"

#define NONE (Operand) { 0 }
#define VREG(v) (Operand) { .type = OPERAND_VREG, .vreg = (v) }
#define IMM(i) (Operand) { .type = OPERAND_IMM, .imm = (i) }
#define MEM(m) (Operand) { .type = OPERAND_MEM, .mem = (m) }
#define INDEX(b, i, o, s) (MemRef) { .base = (b), .index = (i), .offset = (o), .stride = (s), .size = 0 }
#define GLOBAL(g) (Operand) { .type = OPERAND_GLOBAL, .glbl = (g) }
#define SLOT(s) (Operand) { .type = OPERAND_SLOT, .slot = (s) }

const Opcode jump_condition_opcode_table[OP_COUNT] = {
    [OP_EQUALS] = OPCODE_BEQ,
    [OP_NEQUALS] = OPCODE_BNE,
    [OP_LESS_THAN] = OPCODE_BLT,
    [OP_GREATER_THAN] = OPCODE_BGT,
    [OP_LESS_EQUALS] = OPCODE_BLE,
    [OP_GREATER_EQUALS] = OPCODE_BGE,
};


void emit_program(EmitContext *context, Program *into, AST *program) {
    assert(program->type == AST_PROGRAM);

    for (AST **item = program->items; item < program->items + program->count; item++) {
        Procedure proc =  { 0 };
        switch ((*item)->type) {
            case AST_PROCEDURE: emit_procedure(context, &proc, *item); break;
            case AST_EXTERN: continue;
            default:
            UNREACHABLE("not valid program level decl");
            break;
        }

        array_append(*into, proc);
    }
}


void emit_procedure(EmitContext *context, Procedure *into, AST *procedure) {
    assert(procedure->type == AST_PROCEDURE);

    SymbolData *symbol = get_symbol(context->scope, procedure->name->ident);
    ProcSignature *signature = symbol->qtype->signature;

    context->procedure = into;

    context->procedure->name = procedure->name->ident;
    context->procedure->vregs = (VregInfoTable) { 0 };
    context->procedure->slots = (SlotTable) { 0 };
    context->procedure->cfg = (CFGraph) { 0 };

    context->scope = procedure->symbols;

    context->procedure->cfg.entry_block = allocate_block(context);
    start_block(context, context->procedure->cfg.entry_block);

    for (int i = 0; i < signature->param_count; i++) {
        Operand slot = slot_for(context, get_symbol(context->scope, procedure->params[i]->lhs->ident), true);
        emit_instr(context, OPCODE_PARAM, slot, IMM(i), NONE);
    }

    emit_block(context, procedure->body);
    if (context->current_block != -1) emit_ret(context, (Operand) { 0 });

    context->scope = procedure->symbols->prev;
}


void emit_block(EmitContext *context, AST *block) {
    for (int i = 0; i < block->count; i++) {
        emit_statement(context, block->items[i]);
    }
}


void emit_statement(EmitContext *context, AST *statement) {
    switch (statement->type) {
        case AST_CALL:       (void)emit_call(context, statement, false);   break;
        case AST_BINARY_OP:  (void)emit_expr(context, statement);   break;
        case AST_BLOCK:      emit_block(context, statement);  break;
        case AST_DECL:       emit_decl(context, statement);   break;
        case AST_ASSIGNMENT: emit_assign(context, statement); break;
        case AST_RETURN:     emit_return(context, statement); break;
        case AST_IF:         emit_if(context, statement);     break;
        case AST_WHILE:      emit_while(context, statement);  break;
        default:  UNREACHABLE("ASTType");
    }
}



Operand emit_addr(EmitContext *context, AST *lvalue) {
    Operand out = allocate_vreg(context, (VregInfo) {.size = 8, .sign = 0});

    switch (lvalue->type) {
        case AST_SYMBOL: {
            Operand slot = slot_for(context, get_symbol(context->scope, lvalue->ident), false);
            context->procedure->slots.items[slot.slot].address_taken = true;
            emit_instr(context, OPCODE_ADDR,
                       out,
                       slot,
                       NONE);
            break;
        }
        case AST_INDEX: {
            Operand base = emit_addr(context, lvalue->base);
            Operand index = emit_expr(context, lvalue->index);
            index = emit_instr(context, OPCODE_EXT_Z, allocate_vreg(context, (VregInfo){.size=8,.sign=false}), index, NONE);
            int size = quecto_type_size(lvalue->evaled_type);
            emit_instr(context, OPCODE_ADDR,
                       out,
                       MEM(INDEX(base.vreg, index.vreg, 0, size)),
                       NONE);
            break;
        }
        default: UNREACHABLE("invalid lvalue");
    }

    return out;
}


Operand emit_decay(EmitContext *context, AST *of) {
    switch (of->evaled_type->type) {
        case QUECTO_ARRAY:
            return emit_addr(context, of);
        case QUECTO_POINTER:
            return emit_symbol(context, of);
        default:
            assert(0 && "type cannot decay");
            return NONE;
    }
}


Operand emit_lhs(EmitContext *context, AST *lhs) {
    switch (lhs->type) {
        case AST_SYMBOL: {
            SymbolData *var = get_symbol(context->scope, lhs->ident);
            return slot_for(context, var, false);
        }
        case AST_INDEX: {
            Operand base = emit_decay(context, lhs->base);
            Operand index = emit_expr(context, lhs->index);
            index = emit_instr(context, OPCODE_EXT_Z, allocate_vreg(context, (VregInfo){.size=8,.sign=false}), index, NONE);
            int size = quecto_type_size(lhs->evaled_type);
            return MEM(INDEX(base.vreg, index.vreg, 0 , size));
        }
        default: UNREACHABLE("invalid lhs");
    }
}


void emit_list(EmitContext *context, AST *list, Operand *into) {
    for (int i = 0; i < list->count; i++) {
        Operand arg = emit_expr(context, list->items[i]);
        
        emit_instr(context, OPCODE_STORE_INDEX,
                   MEM(INDEX(into->vreg, 0, -i * quecto_type_size(list->evaled_type->inner), 0)),
                   arg,
                   NONE);
    }
}


void emit_decl(EmitContext *context, AST *decl) {
    if (decl->evaled_type->type == QUECTO_ARRAY) {
        Operand lhs = emit_addr(context, decl->lhs);
        emit_list(context, decl->rhs, &lhs);
    } else {
        Operand lhs = emit_lhs(context, decl->lhs);
        Operand rhs = emit_expr(context, decl->rhs);
        emit_instr(context, OPCODE_STORE, lhs, rhs, NONE);
    } 
}


void emit_assign(EmitContext *context, AST *assign) {
    Operand lhs = emit_lhs(context, assign->lhs);
    Operand rhs = emit_expr(context, assign->rhs);
    emit_instr(context, lhs.type == OPERAND_MEM ? OPCODE_STORE_INDEX : OPCODE_STORE, lhs, rhs, NONE);    
}


void emit_if(EmitContext *context, AST *_if) {
    assert(_if->type == AST_IF);

    int end = allocate_block(context);
    int btrue = allocate_block(context);
    int bfalse = _if->otherwise ? allocate_block(context) : end;
    emit_branch(context, btrue, bfalse, _if->condition);
    
    start_block(context, btrue);
    emit_statement(context, _if->then);
    fallthrough_to(context, end);

    if (_if->otherwise) {
        start_block(context, bfalse);
        emit_if_chain(context, _if->otherwise, end);
    }
    start_block(context, end);
}


void emit_if_chain(EmitContext *context, AST *_if, int end) {
    if (_if->type == AST_ELIF) {
        int btrue = allocate_block(context);
        int bfalse = allocate_block(context);

        emit_branch(context, btrue, bfalse, _if->condition);

        start_block(context, btrue);
        emit_statement(context, _if->then);
        fallthrough_to(context, end);

        if (_if->otherwise) {
            start_block(context, bfalse);
            emit_if_chain(context, _if->otherwise, end);
        }
    } else if (_if->type == AST_ELSE) {
        emit_statement(context, _if->then);
        emit_jmp(context, end);
    } else {
        assert(0 && "only elif and else should be chained in 'statement->otherwise' ");
    }
}


void emit_while(EmitContext *context, AST *_while) {
    assert(_while->type == AST_WHILE);

    int condition = allocate_block(context);
    int loop = allocate_block(context);
    int end = allocate_block(context);

    start_block(context, condition);
    emit_branch(context, loop, end, _while->condition);
    start_block(context, loop);
    emit_statement(context, _while->then);
    emit_jmp(context, condition);
    start_block(context, end);
}


void emit_return(EmitContext *context, AST *ret) {
    assert(ret->type == AST_RETURN);
    
    if (ret->rhs)
        emit_ret(context, emit_expr(context, ret->rhs));
    else
        emit_ret(context, (Operand) {0});
}


Operand emit_symbol(EmitContext *context, AST *sym) {
    Operand slot = slot_for(context, get_symbol(context->scope, sym->ident), false);
    switch (sym->evaled_type->type) {
        case QUECTO_ARRAY: return emit_addr(context, sym);
        default: return emit_instr(context, OPCODE_LOAD,
                                   allocate_vreg_for_type(context, sym->evaled_type),
                                   slot,
                                   NONE);
    }
}


Operand emit_call(EmitContext *context, AST *call, bool has_dest) {
    assert(call->type == AST_CALL);

    Operand ops[call->arg_count];
    for (int i = 0; i < call->arg_count; i++) {
        ops[i] = emit_expr(context, call->args[i]);
    }
    for (int i = 0; i < call->arg_count; i++) {
        emit_instr(context, OPCODE_ARG, NONE, ops[i], NONE);
    }
    return emit_instr(context, OPCODE_CALL,
                        has_dest ? allocate_vreg_for_type(context, call->evaled_type) : NONE,
                        global_for(context, call->callee->ident),
                        NONE
                    );
}


Operand emit_binary_op(EmitContext *context, AST *op) {
    assert(op->type == AST_BINARY_OP);
    
    Opcode opcode;
    switch (op->op) {
        case OP_PLUS:           opcode = OPCODE_ADD; break;
        case OP_MINUS:          opcode = OPCODE_SUB; break;
        case OP_MULTIPLY:       opcode = OPCODE_MUL; break;
        case OP_DIVIDE:         opcode = OPCODE_DIV; break;
        case OP_EQUALS:         opcode = OPCODE_CMP_EQ; break;
        case OP_LESS_THAN:      opcode = OPCODE_CMP_LT; break;
        case OP_GREATER_THAN:   opcode = OPCODE_CMP_GT; break;
        case OP_LESS_EQUALS:    opcode = OPCODE_CMP_LEQ; break;
        case OP_GREATER_EQUALS: opcode = OPCODE_CMP_GEQ; break;
        default: UNREACHABLE("invalid operation");
    }
    return emit_instr(context, opcode,
                      allocate_vreg_for_type(context, op->evaled_type),
                      emit_expr(context, op->left),
                      emit_expr(context, op->right)
                    );
}


Operand emit_expr(EmitContext *context, AST *expr) {
    switch (expr->type) {
        case AST_INT_LIT: return emit_instr(context, OPCODE_IMM, allocate_vreg_for_type(context, expr->evaled_type), IMM(expr->int_lit), NONE);
        case AST_SYMBOL: return emit_symbol(context, expr);
        case AST_INDEX: {
            Operand base = emit_decay(context, expr->base);
            Operand index = emit_expr(context, expr->index);
            index = emit_instr(context, OPCODE_EXT_Z, allocate_vreg(context, (VregInfo){.size=8,.sign=false}), index, NONE);
            Operand ref = MEM(INDEX(base.vreg, index.vreg, 0, quecto_type_size(expr->evaled_type)));
            return emit_instr(context, OPCODE_INDEX, allocate_vreg_for_type(context, expr->evaled_type), ref, NONE);
        }
        case AST_REF: return emit_addr(context, expr->base);
        case AST_CALL: return emit_call(context, expr, true);
        case AST_BINARY_OP: return emit_binary_op(context, expr);
        default: UNREACHABLE("invalid expr ast type");
    }
}




Operand emit_instr(EmitContext *context, Opcode opcode, Operand dest, Operand arg1, Operand arg2) {
    Instr instr = { 0 };
    instr.opcode = opcode;
    instr.dest = dest;
    instr.arg1 = arg1;
    instr.arg2 = arg2;
    return emit_instr_into_block(context->arena, &context->procedure->cfg.items[context->current_block], instr);
}


void emit_branch(EmitContext *context, int true_target, int false_target, AST *expr) {
    assert(context->current_block != -1 && expr->type == AST_BINARY_OP && op_is_conditional(expr->op));
    
    Instr terminator = { 0 };
    terminator.opcode = jump_condition_opcode_table[expr->op];
    terminator.arg1 = emit_expr(context, expr->left);
    terminator.arg2 = emit_expr(context, expr->right);

    int size = quecto_type_size(expr->evaled_type);

    if (context->procedure->vregs.items[terminator.arg1.vreg].size < size) terminator.arg1 = emit_instr(context, OPCODE_EXT_Z, allocate_vreg(context, (VregInfo){.size=size,.sign=false}), terminator.arg1, NONE);
    if (context->procedure->vregs.items[terminator.arg2.vreg].size < size) terminator.arg2 = emit_instr(context, OPCODE_EXT_Z, allocate_vreg(context, (VregInfo){.size=size,.sign=false}), terminator.arg2, NONE);

    context->procedure->cfg.items[context->current_block].successors[0] = true_target;
    context->procedure->cfg.items[context->current_block].successors[1] = false_target;

    emit_instr_into_block(context->arena, &context->procedure->cfg.items[context->current_block], terminator);
    context->current_block = -1;
}


void emit_jmp(EmitContext *context, int target) {
    assert(context->current_block != -1);
    
    Instr terminator = { .opcode = OPCODE_JMP };
    context->procedure->cfg.items[context->current_block].successors[0] = target;
    emit_instr_into_block(context->arena, &context->procedure->cfg.items[context->current_block], terminator);

    context->current_block = -1;
}


void emit_ret(EmitContext *context, Operand with) {
    assert(context->current_block != -1);
    
    Instr terminator = { .opcode = OPCODE_RET, .arg1 = with };
    emit_instr_into_block(context->arena, &context->procedure->cfg.items[context->current_block], terminator);

    context->current_block = -1;
}


int allocate_block(EmitContext *context) {
    BasicBlock block = { 0 };
    block.successors[0] = -1;
    block.successors[1] = -1;
    arena_array_append(context->arena, context->procedure->cfg, block);
    return context->procedure->cfg.count - 1;
}


void start_block(EmitContext *context, int block_id) {
    if (context->current_block != -1 && block_id != context->current_block) {
        emit_jmp(context, block_id);
    }
    context->current_block = block_id;
}


void fallthrough_to(EmitContext *context, int block) {
    if (context->current_block != -1) emit_jmp(context, block);
}


Operand allocate_vreg_explicit(Arena *arena, Procedure *procedure, VregInfo info) {
    info.interval.iend = -1;
    info.interval.bend = -1;
    info.interval.istart = -1;
    info.interval.bstart = -1;
    info.color.index = -1;
    info.hint = 0;
    arena_array_append(arena, procedure->vregs, info);
    return VREG(procedure->vregs.count - 1);
}


Operand allocate_vreg(EmitContext *context, VregInfo info) {
    return allocate_vreg_explicit(context->arena, context->procedure, info);
}


Operand allocate_vreg_for_type(EmitContext *context, QuectoType *type) {
  assert(type != NULL && "quecto type not initialized or nulled");

  VregInfo info;
  info.size = quecto_type_size(type);
  info.sign = quecto_is_signed(type);
  info.interval.bend = -1;
  info.interval.iend = -1;
  info.color.index = -1;

  return allocate_vreg(context, info);
}


Operand global_for(EmitContext *context, const char *ident) {
    SymbolTable *globals = context->scope;
    for (;globals->prev != NULL; globals = globals->prev);

    return GLOBAL(ht_nindex(&globals->table, ident, strlen(ident)));
}


Operand slot_for(EmitContext *context, SymbolData *sym, bool param) {
    int *slot = ht_nsearch(&context->slot_from_symbol, &sym, sizeof(SymbolData*)); // pass by pointer cause pointers shud be unique alr
    if (slot != NULL) {
        return SLOT(*slot);
    }

    SlotInfo info = (SlotInfo) {
        .size = quecto_type_size(sym->qtype),
        .sign = quecto_is_signed(sym->qtype),
        .param = param,
        .address_taken = false,
    };

    slot = arena_alloc(context->arena, sizeof(int));
    *slot = context->procedure->slots.count;
    arena_array_append(context->arena, context->procedure->slots, info);

    ht_ninsert(&context->slot_from_symbol, &sym, sizeof(SymbolData*), slot);

    return SLOT(*slot);
}

