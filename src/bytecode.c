// #include <stdio.h>

// #include "ast.h"
// #include "common.h"
#include "bytecode.h"
// #include "symbol_table.h"

// #define VREG(info) (Operand) { .type = OPERAND_VREG, info }
// #define IMM(info) (Operand) { .type = OPERAND_IMM, info }
// #define STACK(info) (Operand) { .type = OPERAND_STACK, .base = -1, .index = -1, .size = 4, info }
// #define STACK_IND(o, i, s) (Operand) { .type = OPERAND_STACK, .base = -1, .stack_offset = (o), .index = (i), .size = (s)}
// #define STACK_AT(b, o) (Operand) {.type = OPERAND_STACK, .base = (b), .stack_offset = (o), .index = -1, .size = 4}

// // NOTE: in the future these instructions will hold more information like
// // size of type, signed or unsigned, etc;

// const Opcode jump_condition_opcode_table[OP_COUNT] = {
//     [OP_EQUALS] = OPCODE_JMP_EQ,
//     [OP_LESS_THAN] = OPCODE_JMP_LT,
//     [OP_GREATER_THAN] = OPCODE_JMP_GT,
//     [OP_LESS_EQUALS] = OPCODE_JMP_LE,
//     [OP_GREATER_EQUALS] = OPCODE_JMP_GE,
// };

// // TODO: the only thing needed to make these work instead of a plethora of gen_x functions
// // is to change the way labels are handled. should have an index into array of names instead
// // of storing the pointer directly, and probably a hash table to make the reverse look up easy?

// Operand gen_instr(Bytecode *bytecode, Opcode opcode, size_t opcount, ...) {
//     assert(opcount <= 3 && "only up to 3 operands are supported currently");
//     va_list args;
//     Instr instr;
    
//     instr.opcode = opcode;
//     instr.dest = (Operand) { 0 };
//     instr.arg1 = (Operand) { 0 };
//     instr.arg2 = (Operand) { 0 };

//     va_start(args, opcount);
    
//     for (int i = 0; i < opcount; i++) {
//         switch (i) {
//             case 0: instr.dest = va_arg(args, Operand); break;
//             case 1: instr.arg1 = va_arg(args, Operand); break;
//             case 2: instr.arg2 = va_arg(args, Operand); break;
//             default: UNREACHABLE("invalid"); break;
//         }
//     }

//     va_end(args);

//     array_append(*bytecode, instr);
//     return instr.dest;
// }

// static int count = 0;

// Operand create_label() {
//     Operand lab;
//     lab.type = OPERAND_LABEL;
//     lab.label_name = calloc(8, sizeof(char));
//     snprintf(lab.label_name, 8, ".LBL%d", count++);
//     return lab;
// }


// Operand create_label_from(const char *str) {
//     Operand lab;
//     lab.type = OPERAND_LABEL;
//     lab.label_name = strdup(str);
//     return lab;
// }


// void gen_label(Bytecode *bytecode, Operand label) {
//     Instr lab;
//     lab.opcode = OPCODE_LABEL;
//     lab.dest = label;
//     array_append(*bytecode, lab);
// }


// int allocate_vreg_explicit(EmitContext *context, VregInfo info) {
//     arena_array_append(context->arena, context->vreg_info, info);
//     return context->vreg_count++;
// }


// int allocate_vreg(EmitContext *context, QuectoType *type) {
//     assert(type != NULL && "quecto type not initialized or nulled");
    
//     VregInfo info;
//     info.size = quecto_type_size(type);
//     info.sign = quecto_is_signed(type);


//     return allocate_vreg_explicit(context, info);
// }


// void emit_program_bytecode(EmitContext *context, Program *program, AST *ast) {
//     for (int i = 0; i < ast->count; i++) {
//         Procedure procedure = {0};
//         AST *current = ast->items[i];

//         switch (current->type) {
//             case AST_PROCEDURE:
//                 emit_procedure_bytecode(context, &procedure, current);
//                 break;
//             case AST_EXTERN:
//                 procedure.externed = true;
//                 procedure.name = current->externed->name->ident;
//                 procedure.param_count = current->externed->param_count;
//                 procedure.return_count = current->externed->return_count;
//                 break;
//             default:
//                 UNREACHABLE("not top-level decl");
//                 break;
//         }

//         array_append(*program, procedure);
//     }
// }


// void emit_procedure_bytecode(EmitContext *context, Procedure *procedure, AST *ast) {
//     assert(ast->type == AST_PROCEDURE);

//     procedure->name = ast->name->ident;
//     procedure->param_count = ast->param_count;
//     procedure->return_count = ast->return_count;

//     context->stack_offset = 0;
//     context->vreg_count = 0;
//     context->vreg_info = (VregInfoTable) {0};
        
//     context->scope = ast->symbols;
//     for (int i = 0; i < ast->param_count; i++) {
//         SymbolData *data = get_symbol(context->scope, ast->params[i]->symbol->ident);

//         context->stack_offset += 4;
//         data->stack_offset = context->stack_offset;
//     }
//     emit_block_bytecode(context, &procedure->bytecode, ast->body);
//     context->scope = ast->symbols->prev;

//     procedure->local_var_size = context->stack_offset;
//     procedure->vreg_count = context->vreg_count;
//     procedure->vreg_info = context->vreg_info;
// }


// void emit_block_bytecode(EmitContext *context, Bytecode *bytecode, AST *ast) {
//     for (int i = 0; i < ast->count; i++) {
//         emit_statement_bytecode(context, bytecode, ast->items[i]);
//     }
// }


// void emit_statement_bytecode(EmitContext *context, Bytecode *bytecode, AST *statement) {
//     switch (statement->type) {
//         case AST_CALL:       (void)emit_call_bytecode(context, bytecode, statement, false);   break;
//         case AST_BLOCK:      emit_block_bytecode(context, bytecode, statement);  break;
//         case AST_DECL:       emit_decl_bytecode(context, bytecode, statement);   break;
//         case AST_ASSIGNMENT: emit_assign_bytecode(context, bytecode, statement); break;
//         case AST_RETURN:     emit_return_bytecode(context, bytecode, statement); break;
//         case AST_IF:         emit_if_bytecode(context, bytecode, statement);     break;
//         case AST_WHILE:      emit_while_bytecode(context, bytecode, statement);  break;
//         case AST_BINARY_OP:  emit_expr_bytecode(context, bytecode, statement);   break;
//         default:  UNREACHABLE("ASTType");
//     }
// }


// void emit_decl_bytecode(EmitContext *context, Bytecode *bytecode, AST *decl) {
//     SymbolData *var = get_symbol(context->scope, decl->symbol->ident);

//     if (decl->evaled_type->type == QUECTO_ARRAY) {
//         int offset = decl->evaled_type->array_size * quecto_type_size(decl->evaled_type->inner);
//         Operand at = gen_instr(bytecode, OPCODE_LOAD_ADDR, 2,
//                                 VREG(.vreg=allocate_vreg_explicit(context, (VregInfo){.size = 8, .sign = false})),
//                                 STACK(.stack_offset = context->stack_offset + offset )
//                             );
//         for (int i = 0; i < decl->evaled_type->array_size; i++) {
//             Operand expr = emit_expr_bytecode(context, bytecode, decl->expr->items[i]);
//             gen_instr(bytecode, OPCODE_STORE, 2, STACK_AT(at.vreg, -i * quecto_type_size(decl->evaled_type->inner)), VREG ( .vreg = expr.vreg ));
//         }

//         context->stack_offset += offset;
//         var->stack_offset = context->stack_offset;
//     } else {
//         context->stack_offset += quecto_type_size(decl->evaled_type);
//         var->stack_offset = context->stack_offset;

//         Operand expr = emit_expr_bytecode(context, bytecode, decl->expr);
//         gen_instr(bytecode, OPCODE_STORE, 2, STACK( .stack_offset = context->stack_offset ), VREG ( .vreg = expr.vreg ));
//     }
// }


// void emit_assign_bytecode(EmitContext *context, Bytecode *bytecode, AST *assignment) {
//     Operand expr = emit_expr_bytecode(context, bytecode, assignment->expr);

//     switch (assignment->symbol->type) {
//         case AST_SYMBOL: {
//             SymbolData *var = get_symbol(context->scope, assignment->symbol->ident);
//             gen_instr(bytecode, OPCODE_STORE, 2, STACK( .stack_offset = var->stack_offset), VREG( .vreg = expr.vreg ));
//             break;    
//         }
//         case AST_INDEX: {
//             SymbolData *var = get_symbol(context->scope, get_underlying_symbol_from(assignment->symbol)->ident);
//             Operand index = emit_expr_bytecode(context, bytecode, assignment->symbol->index);
//             gen_instr(bytecode, OPCODE_STORE, 2,
//                        STACK_IND(var->stack_offset, index.vreg, quecto_type_size(var->qtype->inner)),
//                        VREG( .vreg = expr.vreg));
//             break;
//         }
//         default: UNREACHABLE("invalid assign");
//     }
     
    
// }


// void emit_branch_comp(EmitContext *context, Bytecode *bytecode, Operand label, AST *expr) {// special case of expression
//     if (expr->type == AST_BINARY_OP && op_is_conditional(expr->op)) {
//         Operand left = emit_expr_bytecode(context, bytecode, expr->left);
//         Operand right = emit_expr_bytecode(context, bytecode, expr->right);
//         gen_instr(bytecode, jump_condition_opcode_table[op_opposite(expr->op)], 3, label, left, right);
//     } else {
//         UNREACHABLE("to add...");
//     }
// }


// void emit_if_bytecode(EmitContext *context, Bytecode *bytecode, AST *ifs) {
//     Operand end_label = create_label();
    
//     emit_branch_comp(context, bytecode, end_label, ifs->condition);
//     emit_statement_bytecode(context, bytecode, ifs->then);

//     gen_instr(bytecode, OPCODE_JMP, 1, end_label);

//     if (ifs->otherwise) emit_if_chain(context, bytecode, ifs->otherwise, end_label);
//     gen_label(bytecode, end_label);
// }


// void emit_if_chain(EmitContext *context, Bytecode *bytecode, AST *ast, Operand end_label) {
//     if (ast->type == AST_ELIF) {
//         // Operand expr = emit_expr_bytecode(context, bytecode, ast->condition);
//         Operand label = create_label();
//         emit_branch_comp(context, bytecode, label, ast->condition);

//         emit_statement_bytecode(context, bytecode, ast->then);

//         gen_instr(bytecode, OPCODE_JMP, 1, end_label);
//         gen_label(bytecode, label);
//         if (ast->otherwise) emit_if_chain(context, bytecode, ast->otherwise, end_label);
//         return;
//     } else if (ast->type == AST_ELSE) {
//         emit_block_bytecode(context, bytecode, ast->then);
//         return;
//     }
//     assert(0 && "only elif and else should be chained in 'statement->otherwise' ");
// }


// void emit_while_bytecode(EmitContext *context, Bytecode *bytecode, AST *whiles) {
//     Operand begin_label = create_label();
//     Operand end_label = create_label();

//     gen_label(bytecode, begin_label);

//     emit_branch_comp(context, bytecode, end_label, whiles->condition);    
//     emit_statement_bytecode(context, bytecode, whiles->then);

//     gen_instr(bytecode, OPCODE_JMP, 1, begin_label);
//     gen_label(bytecode, end_label);
// }


// void emit_return_bytecode(EmitContext *context, Bytecode *bytecode, AST *ret) {
//     // TODO: in the future should probably have a return stack similar to function param stack
//     // to support multiple return values
//     Instr instr;
//     instr.opcode = OPCODE_RET;
//     instr.arg1 = emit_expr_bytecode(context, bytecode, ret->expr);
//     array_append(*bytecode, instr);
// }


// Operand emit_expr_bytecode(EmitContext *context, Bytecode *bytecode, AST *expr) {
//     switch(expr->type) {
//         case AST_INT_LIT: return gen_instr(bytecode, OPCODE_LOADI, 2, VREG(.vreg = allocate_vreg(context, expr->evaled_type)), IMM(.imm = expr->int_lit));
//         case AST_SYMBOL: return emit_symbol_bytecode(context, bytecode, expr);
//         case AST_INDEX: {
//             SymbolData *arr = get_symbol(context->scope, expr->access->ident);
//             Operand at = emit_expr_bytecode(context, bytecode, expr->index);
//             return gen_instr(bytecode, OPCODE_LOAD_INDEX, 2,
//                               VREG( .vreg = allocate_vreg(context, expr->evaled_type) ), STACK_IND(arr->stack_offset, at.vreg, quecto_type_size(arr->qtype->inner)));
//         }
//         case AST_CALL: return emit_call_bytecode(context, bytecode, expr, true);
//         case AST_BINARY_OP: return emit_binary_op_bytecode(context, bytecode, expr);
//         default: UNREACHABLE("invalid expr ast type");
//     }
// }


// Operand emit_symbol_bytecode(EmitContext *context, Bytecode* bytecode, AST *symbol) {
//         SymbolData *var = get_symbol(context->scope, symbol->ident);

//         switch (var->qtype->type) {
//             case QUECTO_ARRAY: return gen_instr(bytecode, OPCODE_LOAD_ADDR, 2,
//                      VREG( .vreg = allocate_vreg_explicit(context, (VregInfo) {.sign = false, .size = 8})), STACK( .stack_offset = var->stack_offset ));
//             default: return gen_instr(bytecode, OPCODE_LOAD, 2,
//                      VREG( .vreg = allocate_vreg(context, symbol->evaled_type)), STACK( .stack_offset = var->stack_offset ));
//             case QUECTO_UNKNOWN: UNREACHABLE("cant emit type of unknown"); return (Operand) {0};
//         }
// }


// Operand emit_call_bytecode(EmitContext *context, Bytecode *bytecode, AST *call, bool has_destination) {
//     for (int i = 0; i < call->arg_count; i++) {
//         Operand dest = emit_expr_bytecode(context, bytecode, call->args[i]);
//         Instr param;

//         param.opcode = OPCODE_PARAM;
//         param.arg1 = dest;
//         array_append(*bytecode, param);
//     }

//     SymbolData *procedure = get_symbol(context->scope, call->callee->ident);
//     if (procedure) {
//         Operand label;
//         if (!procedure->externed) label = create_label_from(call->callee->ident);
//         else {
//             char buf[128];
//             snprintf(buf, 128, "_%s", call->callee->ident);
//             label = create_label_from(buf);
//         }
        
//         return gen_instr(bytecode, OPCODE_CALL, 2,
//                       has_destination ? VREG(.vreg = allocate_vreg(context, call->evaled_type)) : (Operand) {0}, label);
//     } else UNREACHABLE("should have been validate in analysis");

//     return (Operand) {0};
// }


// Operand emit_binary_op_bytecode(EmitContext *context, Bytecode *bytecode, AST *op) {
//     Instr instr;
//     instr.dest.type = OPERAND_VREG;
//     instr.dest.vreg = allocate_vreg(context, op->evaled_type);
//     instr.arg1 = emit_expr_bytecode(context, bytecode, op->left);
//     instr.arg2 = emit_expr_bytecode(context, bytecode, op->right);

//     switch (op->op) {
//         case OP_PLUS:           instr.opcode = OPCODE_ADD; break;
//         case OP_MINUS:          instr.opcode = OPCODE_SUB; break;
//         case OP_MULTIPLY:       instr.opcode = OPCODE_MUL; break;
//         case OP_DIVIDE:         instr.opcode = OPCODE_DIV; break;
//         case OP_EQUALS:         instr.opcode = OPCODE_CMP_EQ; break;
//         case OP_LESS_THAN:      instr.opcode = OPCODE_CMP_LT; break;
//         case OP_GREATER_THAN:   instr.opcode = OPCODE_CMP_GT; break;
//         case OP_LESS_EQUALS:    instr.opcode = OPCODE_CMP_LEQ; break;
//         case OP_GREATER_EQUALS: instr.opcode = OPCODE_CMP_GEQ; break;
//         default: UNREACHABLE("invalid operation");
//     }
//     array_append(*bytecode, instr);

//     return instr.dest;
// }


IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode, int vreg_count) {
    IntervalArray intervals;
    array_init(intervals, vreg_count);

    for (int vreg = 0; vreg < vreg_count; vreg++) {
        Interval interval = {0};
        interval.vreg = vreg;
        // find start
        for (int i = 0; i < bytecode.count; i++) {
            if (bytecode.items[i].dest.type == OPERAND_VREG && bytecode.items[i].dest.vreg == vreg) {
                interval.start = i;
                break;
            }
        }

        // find end
        for (int i = bytecode.count - 1; i >= 0; i--) {
            if (bytecode.items[i].arg1.type == OPERAND_VREG && bytecode.items[i].arg1.vreg == vreg) {
                interval.end = i;
                break;
            }
            if (bytecode.items[i].arg2.type == OPERAND_VREG && bytecode.items[i].arg2.vreg == vreg) {
                interval.end = i;
                break;
            }
            if (bytecode.items[i].arg1.type == OPERAND_STACK && bytecode.items[i].arg1.base == vreg) {
                interval.end = i;
                break;
            }
            if (bytecode.items[i].arg1.type == OPERAND_STACK && bytecode.items[i].arg1.base == vreg) {
                interval.end = i;
                break;
            }
        }

        array_append(intervals, interval);
    }

    return intervals;
}

// NOTE: assumes intervals is already sorted
LocationArray linear_scan_register_allocation(IntervalArray *intervals, int vreg_count, PhysRegs *pregs) {
    LocationArray locations = {0};
    bool active_registers[] = { 1, 1, 1, 1, };
    array_init(locations, vreg_count);
    locations.count = vreg_count;
    IntervalArray active = {0};

    for (int i = 0; i < intervals->count; i++) {
        Interval *interval = &intervals->items[i];
        Location location = {0};
        location.type = LOC_REGISTER;
        int vreg = intervals->items[i].vreg;

        // Expire old intervals
        for (int j = 0; j < active.count; j++) {
            if (active.items[j].end > intervals->items[i].start) break;

            // free register
            active_registers[locations.items[active.items[j].vreg].register_index] = true;

            // remove interval from active
            int k = j;
            while (k < active.count - 1) {
                active.items[k] = active.items[k + 1];
                k++;
            }
            active.count--;

        }

        // 4 is the amount of registers currently, will need to be changed for each platform
        if (active.count == 4) {
            // Spill interval i
            assert(0);
        } else {
            // Allocate register
            for (int k = 0; k < 4; k++) {
                if (active_registers[k]) {
                    active_registers[k] = false;
                    location.type = LOC_REGISTER;
                    location.register_index = k;
                    break;
                }
            }

            array_append(active, *interval);
            int insert_slot = active.count - 1;
            while (insert_slot > 0 && active.items[insert_slot - 1].end > interval->end) {
                active.items[insert_slot] = active.items[insert_slot - 1];
                insert_slot--;
            }
            active.items[insert_slot] = *interval;
        }

        locations.items[vreg] = location;
    }
    return locations;
}


// void print_procedure(Procedure procedure) {
//     printf("%s:\n", procedure.name);
//     pretty_print_bytecode(procedure.bytecode);
// }


// void pretty_print_program(Program program) {
//     for (int i = 0; i < program.count; i++) {
//         print_procedure(program.items[i]);
//         printf("\n");
//     }
// }


// void print_live_intervals(IntervalArray intervals) {
//     for (int i = 0; i < intervals.count; i++) {
//         printf("r%d:\t[%d, %d)\n", intervals.items[i].vreg, intervals.items[i].start, intervals.items[i].end);
//     }
// }


// void print_bytecode_op(Operand op) {
//     switch(op.type) {
//         case OPERAND_IMM:      printf("%d", op.imm); break;
//         case OPERAND_VREG:     printf("r%d", op.vreg); break;
//         case OPERAND_LABEL:    printf("%s", op.label_name); break;
//         case OPERAND_STACK:    printf("[rbp - %d]", op.stack_offset); break;
//         default:
//             UNREACHABLE("operand not valid");
//             break;
//     }
// }


// void print_bytecode_branch(const char *name, Operand dst, Operand arg1, Operand arg2) {
//     printf("\t%s ", name);
//     print_bytecode_op(dst);
//     printf(", ");
//     print_bytecode_op(arg1);
//     printf(", ");
//     print_bytecode_op(arg2);
//     printf("\n");
// }

// void print_bytecode_operation(const char *name, Operand dst, Operand arg1, Operand arg2) {
//     printf("\t");
//     print_bytecode_op(dst);
//     printf(" = %s ", name);
//     print_bytecode_op(arg1);
//     printf(", ");
//     print_bytecode_op(arg2);
//     printf("\n");
// }


// void pretty_print_bytecode(Bytecode bytecode) {
//     // TODO: this function needs to be reworked to support
//     // multiple operand types. I need to decide on what types
//     // of operands each instruction can support, and if immediate
//     // arithemtic should have separate instructions or not (leaning towards no for now)
//     for (int i = 0; i < bytecode.count; i++) {
//         Instr instr = bytecode.items[i];
//         printf("%d: ", i);
//         switch (instr.opcode) {
//             case OPCODE_ADD:
//                 print_bytecode_operation("add", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_SUB:
//                 print_bytecode_operation("sub", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_MUL:
//                 print_bytecode_operation("mul", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_DIV:
//                 print_bytecode_operation("div", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_CMP_EQ:
//                 print_bytecode_operation("c.eq", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_CMP_LT:
//                 print_bytecode_operation("c.lt", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_CMP_GT:
//                 print_bytecode_operation("c.gt", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_CMP_LEQ:
//                 print_bytecode_operation("c.le", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_CMP_GEQ:
//                 print_bytecode_operation("c.ge", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_JMP_EQ:
//                 print_bytecode_branch("j.eq", instr.dest, instr.arg1, instr.arg2);
//                     break;
//             case OPCODE_JMP_LT:
//                 print_bytecode_branch("j.lt", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_JMP_GT:
//                 print_bytecode_branch("j.gt", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_JMP_LE:
//                 print_bytecode_branch("j.le", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_JMP_GE:
//                 print_bytecode_branch("j.ge", instr.dest, instr.arg1, instr.arg2);
//                 break;
//             case OPCODE_LOAD_INDEX:
//                 printf("\tr%d = [bp + r%d - %d]\n", instr.dest.vreg, instr.arg1.index,  instr.arg1.stack_offset);
//                 break;
//             case OPCODE_LOAD:
//                 printf("\tr%d = [bp - %d]\n", instr.dest.vreg, instr.arg1.stack_offset);
//                 break;
//             case OPCODE_STORE:
//                 printf("\t[bp - %d] = r%d\n", instr.dest.stack_offset, instr.arg1.vreg);
//                 break;
//             case OPCODE_COPY:
//                 printf("\t");
//                 print_bytecode_op(instr.dest);
//                 printf(" = ");
//                 print_bytecode_op(instr.arg1);
//                 printf("\n");
//                 break;
//             case OPCODE_LOADI:
//                 printf("\t");
//                 print_bytecode_op(instr.dest);
//                 printf(" = loadi ");
//                 print_bytecode_op(instr.arg1);
//                 printf("\n");
//                 break;
//             case OPCODE_RET:
//                 printf("\tret r%d\n", instr.arg1.vreg);
//                 break;
//             case OPCODE_JMP:
//                 printf("\tjmp %s\n", instr.dest.label_name);
//                 break;
//             case OPCODE_JMP_NEQ:
//                 printf("\tj.ne r%d, %s\n", instr.arg1.vreg, instr.dest.label_name);
//                 break;
//             case OPCODE_PARAM:
//                 printf("\tparam r%d\n", instr.arg1.vreg);
//                 break;
//             case OPCODE_CALL:
//                 if (instr.dest.type == OPERAND_INVALID)
//                     printf("\tcall %s\n", instr.arg1.label_name);
//                 else
//                     printf("\tr%d = call %s\n", instr.dest.vreg, instr.arg1.label_name);
//                 break;
//             case OPCODE_LABEL:
//                 printf("%s:\n", instr.dest.label_name);
//                 break;
//             default:
//                 printf("error unrecognized instruction in bytecode\n");
//         }
//     }
// }

