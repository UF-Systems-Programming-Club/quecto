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

    SymbolData *info = get_symbol(context->scope, procedure->name->ident);

    context->vregs = (VregInfoTable) { 0 };
    context->slots = (SlotTable) { 0 };
    context->cfg = (CFGraph) { 0 };

    context->scope = procedure->symbols;

    for (int i = 0; i < info->param_count; i++) {
        (void)slot_for(context, get_symbol(context->scope, procedure->params[i]->lhs->ident));
    }

    context->cfg.entry_block = allocate_block(context);
    start_block(context, context->cfg.entry_block);
    emit_block(context, procedure->body);
    if (context->current_block != -1) emit_ret(context, (Operand) { 0 });

    context->scope = procedure->symbols->prev;

    add_predecessor(&context->cfg, context->cfg.entry_block, -1);

    dominance(context->arena, &context->cfg);

    into->name = procedure->name->ident;
    into->cfg = context->cfg;
    into->vregs = context->vregs;
    into->slots = context->slots;
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
    Operand out = allocate_vreg_explicit(context, (VregInfo) {.size = 8, .sign = 0});

    switch (lvalue->type) {
        case AST_SYMBOL: {
            Operand slot = slot_for(context, get_symbol(context->scope, lvalue->ident));
            context->slots.items[slot.slot].address_taken = true;
            emit_instr(context, OPCODE_ADDR, 2, out, slot);
            break;
        }
        case AST_INDEX: {
            Operand base = emit_addr(context, lvalue->array);
            Operand index = emit_expr(context, lvalue->index);
            int size = quecto_type_size(lvalue->evaled_type);
            emit_instr(context, OPCODE_ADDR, 2,
                       out, MEM(INDEX(AT_VR(base.vreg), AT_VR(index.vreg), 0, size)));
            break;
        }
        default: UNREACHABLE("invalid lvalue");
    }

    return out;
}


Operand emit_lhs(EmitContext *context, AST *lhs) {
    switch (lhs->type) {
        case AST_SYMBOL: {
            SymbolData *var = get_symbol(context->scope, lhs->ident);
            return slot_for(context, var);
        }
        case AST_INDEX: {
            Operand base = emit_addr(context, lhs->array);
            Operand index = emit_expr(context, lhs->index);
            int size = quecto_type_size(lhs->evaled_type);
            return MEM(INDEX(AT_VR(base.vreg), AT_VR(index.vreg), 0 , size));
        }
        default: UNREACHABLE("invalid lhs");
    }
}


void emit_list(EmitContext *context, AST *list, Operand *into) {
    for (int i = 0; i < list->count; i++) {
        Operand arg = emit_expr(context, list->items[i]);
        
        emit_instr(context, OPCODE_STORE, 2,
                   MEM(INDEX(AT_VR(into->vreg), 0, -i * quecto_type_size(list->evaled_type->inner), 0)), arg);
    }
}


void emit_decl(EmitContext *context, AST *decl) {
    if (decl->evaled_type->type == QUECTO_ARRAY) {
        Operand lhs = emit_addr(context, decl->lhs);
        emit_list(context, decl->rhs, &lhs);
    } else {
        Operand lhs = emit_lhs(context, decl->lhs);
        Operand rhs = emit_expr(context, decl->rhs);
        emit_instr(context, OPCODE_STORE, 2, lhs, rhs);
    } 
}


void emit_assign(EmitContext *context, AST *assign) {
    Operand lhs = emit_lhs(context, assign->lhs);
    Operand rhs = emit_expr(context, assign->rhs);
    emit_instr(context, OPCODE_STORE, 2, lhs, rhs);    
}


const Opcode jump_condition_opcode_table[OP_COUNT] = {
    [OP_EQUALS] = OPCODE_BEQ,
    [OP_NEQUALS] = OPCODE_BNE,
    [OP_LESS_THAN] = OPCODE_BLT,
    [OP_GREATER_THAN] = OPCODE_BGT,
    [OP_LESS_EQUALS] = OPCODE_BLE,
    [OP_GREATER_EQUALS] = OPCODE_BGE,
};


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
    Operand slot = slot_for(context, get_symbol(context->scope, sym->ident));
    switch (sym->evaled_type->type) {
        case QUECTO_ARRAY: return emit_addr(context, sym);
        default: return emit_instr(context, OPCODE_LOAD, 2,
                allocate_vreg_type(context, sym->evaled_type), slot);
    }
}


Operand emit_call(EmitContext *context, AST *call, bool has_dest) {
    assert(call->type == AST_CALL);
    
    for (int i = 0; i < call->arg_count; i++) {
        emit_instr(context, OPCODE_PARAM, 2, (Operand) {.type = OPERAND_NONE},
                   emit_expr(context, call->args[i]));
    }
    return emit_instr(context, OPCODE_CALL, 2, has_dest ?
                        allocate_vreg_type(context, call->evaled_type) : (Operand) {0},
                        global_for(context, call->callee->ident)
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
    return emit_instr(context, opcode, 3,
                      allocate_vreg_type(context, op->evaled_type),
                      emit_expr(context, op->left),
                      emit_expr(context, op->right)
                    );
}


Operand emit_expr(EmitContext *context, AST *expr) {
    switch (expr->type) {
        case AST_INT_LIT: return emit_instr(context, OPCODE_IMM, 2, allocate_vreg_type(context, expr->evaled_type), IMM(expr->int_lit));
        case AST_SYMBOL: return emit_symbol(context, expr);
        case AST_INDEX: {
            Operand base = emit_addr(context, expr->array);
            Operand index = emit_expr(context, expr->index);
            Operand ref = MEM(INDEX(AT_VR(base.vreg), AT_VR(index.vreg), 0, quecto_type_size(expr->evaled_type)));
            return emit_instr(context, OPCODE_INDEX, 2, allocate_vreg_type(context, expr->evaled_type), ref);
        }
        case AST_CALL: return emit_call(context, expr, true);
        case AST_BINARY_OP: return emit_binary_op(context, expr);
        default: UNREACHABLE("invalid expr ast type");
    }
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


void emit_branch(EmitContext *context, int true_target, int false_target, AST *expr) {
    assert(context->current_block != -1 && expr->type == AST_BINARY_OP && op_is_conditional(expr->op));
    
    Instr terminator = { .opcode = jump_condition_opcode_table[expr->op], .successor = { true_target, false_target} };
    terminator.arg1 = emit_expr(context, expr->left);
    terminator.arg2 = emit_expr(context, expr->right);

    context->cfg.items[context->current_block].terminator = terminator;
    context->current_block = -1;
}


void emit_jmp(EmitContext *context, int target) {
    assert(context->current_block != -1);
    
    Instr terminator = { .opcode = OPCODE_JMP, .successor = { target, -1} };

    context->cfg.items[context->current_block].terminator = terminator;
    context->current_block = -1;
}


void emit_ret(EmitContext *context, Operand with) {
    assert(context->current_block != -1);
    
    Instr terminator = { .opcode = OPCODE_RET, .arg1 = with , .successor = { -1, -1} };

    context->cfg.items[context->current_block].terminator = terminator;
    context->current_block = -1;
}


Operand emit_instr(EmitContext *context, Opcode opcode, size_t opcount, ...) {
  assert(opcount <= 3 && "only up to 3 operands are supported currently");
  assert(context->current_block != -1);

  va_list args;
  Instr instr;
  
  instr.opcode = opcode;
  instr.successor[0] = -1; instr.successor[1] = -1;
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

  arena_array_append(context->arena, context->cfg.items[context->current_block].bytecode, instr);
  return instr.dest;
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

    assert(sym != NULL); // not in globals;
    
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


int allocate_block(EmitContext *context) {
    arena_array_append(context->arena, context->cfg, (BasicBlock){ 0 });
    return context->cfg.count - 1;
}


void sweep_slots(Procedure *procedure) {
    for (int i = 0; i < procedure->slots.count; i++) {
        if (procedure->slots.items[i].address_taken) {
            
        }
    }
    for (int i = 0; i < procedure->cfg.count; i++) {
        if (procedure->cfg.items[i].bytecode.count == 0) continue;

        
    }
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


bool print_operand(Operand operand, bool leading) {
    if (operand.type != OPERAND_NONE && leading) printf(", ");
    switch (operand.type) {
        case OPERAND_VREG: printf("r%d", operand.vreg); break;
        case OPERAND_REG: printf("%s", reg_str[operand.reg]); break;
        case OPERAND_MEM: print_mem(operand.mem); break;
        case OPERAND_IMM: printf("%d", operand.imm); break;
        case OPERAND_SLOT: printf("s%d", operand.slot); break;
        case OPERAND_GLOBAL: printf("GBL%d", operand.glbl); break;
        default: return false;
    }
    return true;
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
    [OPCODE_BEQ]  = "beq",
    [OPCODE_BLT]  = "blt",
    [OPCODE_BGT]  = "bgt",
    [OPCODE_BGE]  = "bge",
    [OPCODE_BLE]  = "ble",
    [OPCODE_BNE] = "bne",
    [OPCODE_CALL]    = "call",
    [OPCODE_PARAM]   = "param",
};


void print_terminator(Instr instr) {
    printf("\t%s ", op_str[instr.opcode]);
    if (instr.opcode == OPCODE_JMP) printf("#%d ", instr.successor[0]);
    else if (instr.opcode != OPCODE_RET)
printf("(t: #%d, f: #%d) ", instr.successor[0], instr.successor[1]);
    print_operand(instr.arg2, print_operand(instr.arg1, print_operand(instr.dest, false)));;
    printf("\n");
}


void print_instruction(Instr instr) {
    if (OPCODE_COPY <= instr.opcode && instr.opcode <= OPCODE_PARAM) {
        printf("\t%s ", op_str[instr.opcode]);
        print_operand(instr.arg2, print_operand(instr.arg1, print_operand(instr.dest, false)));;
        printf("\n");
    } else {
        printf("\t");
        print_operand(instr.dest, false);
        printf(" = %s ", op_str[instr.opcode]);
        print_operand(instr.arg2, print_operand(instr.arg1, false));
        printf("\n");
    }
}


void print_block(CFGraph graph, bool walked[], int block) {
    if (block == -1 || walked[block]) return;

    
    BasicBlock b =  graph.items[block];
    
    printf("block #%d:\n", block);
    walked[block] = true;

    for (int i = 0; i < b.bytecode.count; i++) {
        printf("\t");
        print_instruction(b.bytecode.items[i]);
    }

    printf("\t");
    print_terminator(b.terminator);
    
    print_block(graph, walked, b.terminator.successor[0]);
    print_block(graph, walked, b.terminator.successor[1]);
}


void print_cfg(CFGraph graph) {
    bool *walked = calloc(graph.count, sizeof(bool));
    print_block(graph, walked, graph.entry_block);
    free(walked);
}


void print_procedure(Procedure procedure) {
    printf("%s:\n", procedure.name);
    print_cfg(procedure.cfg);
}


void print_program(Program program) {
    for (int i = 0; i < program.count; i++) {
        print_procedure(program.items[i]);
        puts("\n\n");
    }
}
