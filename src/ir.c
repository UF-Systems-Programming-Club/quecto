#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "ir.h"
#include "symbol_table.h"

#define VREG(v) (Operand) { .type = OPERAND_VREG, .vreg = (v) }
#define IMM(i) (Operand) { .type = OPERAND_IMM, .imm = (i) }
#define MEM(m) (Operand) { .type = OPERAND_MEM, .mem = (m) }
#define SLOT(s) (Operand) { .type = OPERAND_SLOT, .slot = (s) }
#define GLOBAL(g) (Operand) { .type = OPERAND_GLOBAL, .glbl = (g) }
#define LOCAL(l) (Operand) { .type = OPERAND_LABEL, .label_id = (l) }
#define AT_BP (Addr) { .type = ADDR_REG, .reg = PR_BP }
#define AT_VR(v) (Addr) { .type = ADDR_VREG, .vreg = v }
#define STK(o) (MemRef) { .base = AT_BP, .offset = (o), .size = (0), .index = (0) }
#define INDEX(b, i, o, s) (MemRef) { .base = (b), .index = (i), .offset = (o), .stride = (s), .size = 0 }

void emit_program(EmitContext *context, Program *into, AST *program) {
    assert(program->type == AST_PROGRAM);

    for (AST **item = program->items; item < program->items + program->count; item++) {
        Procedure proc =  { 0 };
        switch ((*item)->type) {
            case AST_PROCEDURE: emit_procedure(context, &proc, *item); break;
            case AST_EXTERN: break;
            default:
            UNREACHABLE("not valid program level decl");
            break;
        }

        array_append(*into, proc);
    }
}


void emit_procedure(EmitContext *context, Procedure *into, AST *procedure) {
  assert(procedure->type == AST_PROCEDURE);

  SymbolData *info = get_symbol(context->scope, procedure->name->ident);

  context->vregs = (VregInfoTable) { 0 };
  context->labels = (LabelTable) { 0 };
  context->slots = (SlotTable) { 0 };

  context->scope = procedure->symbols;

  for (int i = 0; i < info->param_count; i++) {
    (void)slot_for(context, get_symbol(context->scope, procedure->params[i]->lhs->ident));
  }

  emit_block(context, &into->bytecode, procedure->body);

  context->scope = procedure->symbols->prev;

  print_procedure(*into);

  into->vregs = context->vregs;
  into->labels = context->labels;
  into->slots = context->slots;
}


void emit_block(EmitContext *context, Bytecode *bytecode, AST *block) {
  for (int i = 0; i < block->count; i++)
    emit_statement(context, bytecode, block->items[i]);
}


void emit_statement(EmitContext *context, Bytecode *bytecode, AST *statement) {
    switch (statement->type) {
        case AST_CALL:       (void)emit_call(context, bytecode, statement, false);   break;
        case AST_BINARY_OP:  (void)emit_expr(context, bytecode, statement);   break;
        case AST_BLOCK:      emit_block(context, bytecode, statement);  break;
        case AST_DECL:       emit_decl(context, bytecode, statement);   break;
        case AST_ASSIGNMENT: emit_assign(context, bytecode, statement); break;
        case AST_RETURN:     emit_return(context, bytecode, statement); break;
        case AST_IF:         emit_if(context, bytecode, statement);     break;
        case AST_WHILE:      emit_while(context, bytecode, statement);  break;
        default:  UNREACHABLE("ASTType");
    }
}


Operand emit_addr(EmitContext *context, Bytecode *bytecode, AST *lvalue) {
    Operand out = allocate_vreg_explicit(context, (VregInfo) {.size = 8, .sign = 0});

    switch (lvalue->type) {
        case AST_SYMBOL: {
            Operand slot = slot_for(context, get_symbol(context->scope, lvalue->ident));
            context->slots.items[slot.slot].address_taken = true;
            gen_instr(bytecode, OPCODE_ADDR, 2, out, slot);
            break;
        }
        case AST_INDEX: {
            Operand base = emit_addr(context, bytecode, lvalue->array);
            Operand index = emit_expr(context, bytecode, lvalue->index);
            int size = quecto_type_size(lvalue->evaled_type);
            gen_instr(bytecode, OPCODE_ADDR, 2,
                       out, MEM(INDEX(AT_VR(base.vreg), AT_VR(index.vreg), 0, size)));
            break;
        }
        default: UNREACHABLE("invalid lvalue");
    }

    return out;
}


Operand emit_lhs(EmitContext *context, Bytecode *bytecode, AST *lhs) {
    switch (lhs->type) {
        case AST_SYMBOL: {
            SymbolData *var = get_symbol(context->scope, lhs->ident);
            return slot_for(context, var);
        }
        case AST_INDEX: {
            Operand base = emit_addr(context, bytecode, lhs->array);
            Operand index = emit_expr(context, bytecode, lhs->index);
            int size = quecto_type_size(lhs->evaled_type);
            return MEM(INDEX(AT_VR(base.vreg), AT_VR(index.vreg), 0 , size));
        }
        default: UNREACHABLE("invalid lhs");
    }
}


void emit_list(EmitContext *context, Bytecode *bytecode, AST *list, Operand *into) {
    for (int i = 0; i < list->count; i++) {
        Operand arg = emit_expr(context, bytecode, list->items[i]);
        
        gen_instr(bytecode, OPCODE_STORE, 2,
                   MEM(INDEX(AT_VR(into->vreg), 0, -i * quecto_type_size(list->evaled_type->inner), 0)), arg);
    }
}


void emit_decl(EmitContext *context, Bytecode *bytecode, AST *decl) {
    if (decl->evaled_type->type == QUECTO_ARRAY) {
        Operand lhs = emit_addr(context, bytecode, decl->lhs);
        emit_list(context, bytecode, decl->rhs, &lhs);
    } else {
        Operand lhs = emit_lhs(context, bytecode, decl->lhs);
        Operand rhs = emit_expr(context, bytecode, decl->rhs);
        gen_instr(bytecode, OPCODE_STORE, 2, lhs, rhs);
    } 
}


void emit_assign(EmitContext *context, Bytecode *bytecode, AST *assign) {
    Operand lhs = emit_lhs(context, bytecode, assign->lhs);
    Operand rhs = emit_expr(context, bytecode, assign->rhs);
    gen_instr(bytecode, OPCODE_STORE, 2, lhs, rhs);    
}


const Opcode jump_condition_opcode_table[OP_COUNT] = {
    [OP_EQUALS] = OPCODE_JMP_EQ,
    [OP_LESS_THAN] = OPCODE_JMP_LT,
    [OP_GREATER_THAN] = OPCODE_JMP_GT,
    [OP_LESS_EQUALS] = OPCODE_JMP_LE,
    [OP_GREATER_EQUALS] = OPCODE_JMP_GE,
};


void emit_branch_comp(EmitContext *context, Bytecode *bytecode, Operand label, AST *expr) {
    assert(expr->type == AST_BINARY_OP && op_is_conditional(expr->op));
    
    Operand left = emit_expr(context, bytecode, expr->left);
    Operand right = emit_expr(context, bytecode, expr->right);
    gen_instr(bytecode, jump_condition_opcode_table[op_opposite(expr->op)], 3, label, left, right);
}


void emit_if(EmitContext *context, Bytecode *bytecode, AST *_if) {
    Operand end_label = allocate_label(context, bytecode);

    emit_branch_comp(context, bytecode, end_label, _if->condition);
    emit_statement(context, bytecode, _if->then);

    gen_instr(bytecode, OPCODE_JMP, 1, end_label);

    if (_if->otherwise)
        emit_if_chain(context, bytecode, _if->otherwise, end_label);

    force_label_here(context, bytecode, end_label);
}


void emit_if_chain(EmitContext *context, Bytecode *bytecode, AST *_if, Operand end_label) {
    if (_if->type == AST_ELIF) {
        Operand label = allocate_label(context, bytecode);

        emit_branch_comp(context, bytecode, label, _if->condition);
        emit_statement(context, bytecode, _if->then);

        gen_instr(bytecode, OPCODE_JMP, 1, end_label);
        force_label_here(context, bytecode, label);

        if (_if->otherwise)
            emit_if_chain(context, bytecode, _if->otherwise, end_label);
    } else if (_if->type == AST_ELSE) {
        emit_statement(context, bytecode, _if->then);
    } else {
        assert(0 && "only elif and else should be chained in 'statement->otherwise' ");
    }
}


void emit_while(EmitContext *context, Bytecode *bytecode, AST *_while) {
    Operand begin_label = allocate_label(context, bytecode);
    Operand end_label = allocate_label(context, bytecode);

    emit_branch_comp(context, bytecode, end_label, _while->condition);    
    emit_statement(context, bytecode, _while->then);

    gen_instr(bytecode, OPCODE_JMP, 1, begin_label);
    force_label_here(context, bytecode, end_label);
}


void emit_return(EmitContext *context, Bytecode *bytecode, AST *ret) {
    if (ret->rhs)
        gen_instr(bytecode, OPCODE_RET, 2, (Operand) {.type = OPERAND_NONE}, emit_expr(context, bytecode, ret->rhs));
    else
        gen_instr(bytecode, OPCODE_RET, 0);
}


Operand emit_symbol(EmitContext *context, Bytecode *bytecode, AST *sym) {
    Operand slot = slot_for(context, get_symbol(context->scope, sym->ident));
    switch (sym->evaled_type->type) {
        case QUECTO_ARRAY: return emit_addr(context, bytecode, sym);
        default: return gen_instr(bytecode, OPCODE_LOAD, 2,
                allocate_vreg_type(context, sym->evaled_type), slot);
    }
}


Operand emit_call(EmitContext *context, Bytecode *bytecode, AST *call, bool has_dest) {
    for (int i = 0; i < call->arg_count; i++) {
        gen_instr(bytecode, OPCODE_PARAM, 2, (Operand) {.type = OPERAND_NONE},
                   emit_expr(context, bytecode, call->args[i]));
    }
    return gen_instr(bytecode, OPCODE_CALL, 2, has_dest ?
                        allocate_vreg_type(context, call->evaled_type) : (Operand) {0},
                        global_for(context, call->callee->ident)
                    );
}


Operand emit_binary_op(EmitContext *context, Bytecode *bytecode, AST *op) {
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
    return gen_instr(bytecode, opcode, 3,
                      allocate_vreg_type(context, op->evaled_type),
                      emit_expr(context, bytecode, op->left),
                      emit_expr(context, bytecode, op->right)
                    );
}


Operand emit_expr(EmitContext *context, Bytecode *bytecode, AST *expr) {
    switch (expr->type) {
        case AST_INT_LIT: return gen_instr(bytecode, OPCODE_IMM, 2, allocate_vreg_type(context, expr->evaled_type), IMM(expr->int_lit));
        case AST_SYMBOL: return emit_symbol(context, bytecode, expr);
        case AST_INDEX: {
            Operand base = emit_addr(context, bytecode, expr->array);
            Operand index = emit_expr(context, bytecode, expr->index);
            Operand ref = MEM(INDEX(AT_VR(base.vreg), AT_VR(index.vreg), 0, quecto_type_size(expr->evaled_type)));
            return gen_instr(bytecode, OPCODE_INDEX, 2, allocate_vreg_type(context, expr->evaled_type), ref);
        }
        case AST_CALL: return emit_call(context, bytecode, expr, true);
        case AST_BINARY_OP: return emit_binary_op(context, bytecode, expr);
        default: UNREACHABLE("invalid expr ast type");
    }
}


Operand gen_instr(Bytecode *bytecode, Opcode opcode, size_t opcount, ...) {
  assert(opcount <= 3 && "only up to 3 operands are supported currently");
  va_list args;
  Instr instr;
  
  instr.opcode = opcode;
  instr.dest = (Operand) { 0 };
  instr.arg1 = (Operand) { 0 };
  instr.arg2 = (Operand) { 0 };

  va_start(args, opcount);
  
  for (int i = 0; i < opcount; i++) {
      switch (i) {
          case 0: instr.dest = va_arg(args, Operand); break;
          case 1: instr.arg1 = va_arg(args, Operand); break;
          case 2: instr.arg2 = va_arg(args, Operand); break;
          default: UNREACHABLE("invalid"); break;
      }
  }

  va_end(args);

  array_append(*bytecode, instr);
  return instr.dest;
}


Operand allocate_label(EmitContext *context, Bytecode *bytecode) {
  arena_array_append(context->arena, context->labels, bytecode->count);
  return LOCAL(context->labels.count - 1);
}


void force_label_here(EmitContext *context, Bytecode *bytecode, Operand label) {
    context->labels.items[label.label_id] = bytecode->count;
}


Operand allocate_vreg_explicit(EmitContext *context, VregInfo info) {
  arena_array_append(context->arena, context->vregs, info);
  return VREG(context->vregs.count - 1);
}


Operand allocate_vreg_type(EmitContext *context, QuectoType *type) {
  assert(type != NULL && "quecto type not initialized or nulled");

  VregInfo info;
  info.size = quecto_type_size(type);
  info.sign = quecto_is_signed(type);

  return allocate_vreg_explicit(context, info);
}


Operand global_for(EmitContext *context, const char *ident) {
    SymbolTable *globals = context->scope;
    for (;globals->prev != NULL; globals = globals->prev);
    SymbolData *sym = get_symbol(globals, ident);
    return GLOBAL(sym->id);
}


Operand slot_for(EmitContext *context, SymbolData *sym) {
    int *slot = ht_nsearch(&context->slot_from_symbol, &sym, sizeof(SymbolData*)); // pass by pointer cause pointers shud be unique alr
    if (slot != NULL) {
        return SLOT(*slot);
    }

    SlotInfo info = (SlotInfo) {
        .size = quecto_type_size(sym->qtype),
        .sign = quecto_is_signed(sym->qtype),
        .address_taken = false,
    };

    slot = arena_alloc(context->arena, sizeof(int));
    *slot = context->slots.count;
    arena_array_append(context->arena, context->slots, info);

    ht_ninsert(&context->slot_from_symbol, &sym, sizeof(SymbolData*), slot);

    return SLOT(*slot);
}

const char *reg_str[PR_BP + 1] = {
    [PR_BP] = "bp",
};

void print_addr(Addr addr) {
    switch (addr.type) {
        case ADDR_VREG: printf("r%d", addr.vreg); break;
        case ADDR_REG: printf("%s", reg_str[addr.reg]); break;
        case ADDR_NONE: break;
    }
}

void print_mem(MemRef mem) {
    printf("["); print_addr(mem.base);
    if (mem.stride > 0) {
        printf(" + %d * ", mem.stride);
        print_addr(mem.index);
    }
    if (mem.offset > 0) printf(" - %d", abs(mem.offset));
    else if (mem.offset < 0) printf(" + %d", abs(mem.offset));
    printf("]");
}

void print_operand(Operand operand) {
    switch (operand.type) {
        case OPERAND_VREG: printf("r%d", operand.vreg); break;
        case OPERAND_REG: printf("%s", reg_str[operand.reg]); break;
        case OPERAND_MEM: print_mem(operand.mem); break;
        case OPERAND_IMM: printf("%d", operand.imm); break;
        case OPERAND_SLOT: printf("s%d", operand.slot); break;
        case OPERAND_LABEL: printf("LBL%d", operand.label_id); break;
        case OPERAND_GLOBAL: printf("GBL%d", operand.glbl); break;
        case OPERAND_NONE: break;
    }
}

const char *op_str[OPCODE_COUNT] = {
    [OPCODE_ADD]     = "add",
    [OPCODE_SUB]     = "sub",
    [OPCODE_MUL]     = "mul",
    [OPCODE_DIV]     = "div",
    [OPCODE_CMP_EQ]  = "ceq",
    [OPCODE_CMP_LT]  = "clt",
    [OPCODE_CMP_GT]  = "cgt",
    [OPCODE_CMP_LEQ] = "cle",
    [OPCODE_CMP_GEQ] = "cge",
    [OPCODE_LOAD]    = "load",
    [OPCODE_IMM]     = "imm",
    [OPCODE_INDEX]   = "index",
    [OPCODE_ADDR]    = "addr",
    [OPCODE_STORE]   = "store",
    [OPCODE_COPY]    = "copy",
    [OPCODE_RET]     = "ret",
    [OPCODE_JMP]     = "jmp",
    [OPCODE_JMP_EQ]  = "jeq",
    [OPCODE_JMP_LT]  = "jlt",
    [OPCODE_JMP_GT]  = "jgt",
    [OPCODE_JMP_GE]  = "jge",
    [OPCODE_JMP_LE]  = "jle",
    [OPCODE_JMP_NEQ] = "jne",
    [OPCODE_CALL]    = "call",
    [OPCODE_PARAM]   = "param",
};

void print_instruction(Instr instr) {
    if (OPCODE_COPY <= instr.opcode && instr.opcode <= OPCODE_PARAM) {
        printf("%s ", op_str[instr.opcode]);
        print_operand(instr.dest); printf(", ");
        print_operand(instr.arg1); printf(", ");
        print_operand(instr.arg2); printf("\n");
    } else {
        print_operand(instr.dest);
        printf(" = %s ", op_str[instr.opcode]);
        print_operand(instr.arg1); printf(", ");
        print_operand(instr.arg2); printf("\n");
    }
}


void print_procedure(Procedure procedure) {
    for (int i = 0; i < procedure.bytecode.count; i++) {
        print_instruction(procedure.bytecode.items[i]);
    }
}
