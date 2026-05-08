#include "passes.h"
#include "cfg.h"


void passes_run(Program *program, Passes passes, Arenas arenas) {
    for (int i = 0; i < program->count; i++) {
        Procedure *procedure = &program->items[i];
        passes_preparation(&arenas, procedure);

        for (int j = 0; passes[j] != NULL; j++) {
            passes[j](&arenas, procedure);
        }
    }
}


void passes_preparation(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;

    allocate_cfg(arenas->persistent, cfg);
    
    int i = 0;
    fill_rpo(cfg, &i, procedure->cfg.entry_block);
    fill_predecessors(cfg);
    fill_idom(cfg); // needs rpo, rpo_list, predecessors
    fill_df(cfg);
    pass_liveness(arenas, procedure); // needs vreg info
}


void pass_liveness(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    VregInfoTable *vregs = &procedure->vregs;

    for (int i = 0; i < cfg->count; i++) {
        set_create(&cfg->live_in[i], arenas->persistent, vregs->count);
        set_create(&cfg->live_out[i], arenas->persistent, vregs->count);
        set_create(&cfg->uses[i], arenas->persistent, vregs->count);
        set_create(&cfg->defines[i], arenas->persistent, vregs->count);
    }

    Set tmp;
    set_create(&tmp, arenas->scratch, vregs->count);
    
    for (int b = 0; b < cfg->count; b++) {
        BasicBlock *block = &cfg->items[b];
        Instr instr;

        for (int i = 0; i < block->bytecode.count; i++) {
            instr = block->bytecode.items[i];
            if (instr.dest.type == OPERAND_VREG) {
                set_insert(&cfg->defines[b], instr.dest.vreg);
            }
            int used[16];
            int count = vreg_if_use(instr, used);
            for (int v = 0; v < count; v++) {
                if (!set_has(&cfg->defines[b], used[v])) {
                    set_insert(&cfg->uses[b], used[v]);
                }
            }
        }
        
        instr = block->terminator;
        if (instr.arg1.type == OPERAND_VREG) {
            if (!set_has(&cfg->defines[b], instr.arg1.vreg))
                set_insert(&cfg->uses[b], instr.arg1.vreg);
        }
        if (instr.arg2.type == OPERAND_VREG) {
            if (!set_has(&cfg->defines[b], instr.arg2.vreg))
                set_insert(&cfg->uses[b], instr.arg2.vreg);
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int k = 0; k < cfg->count; k++) {
            int b = cfg->rpo_list[k];
            BasicBlock *block = &cfg->items[b];

            for (int s = 0; s < 2; s++) {
                int succ = block->terminator.successor[s];

                if (succ == -1) continue;
                set_add(&tmp, &cfg->live_in[succ]);
                
                BasicBlock *successor = &cfg->items[succ];
                int idx = -1;
                for (int i = 0; i < successor->predecessors.count; i++) {
                    if (b == successor->predecessors.items[i]) idx = i;
                }
                for (int p = 0; p < successor->phis.count; p++) {
                    Phi phi = successor->phis.items[p];
                    if (phi.args[idx].type == OPERAND_VREG)
                        set_insert(&tmp, phi.args[idx].vreg);
                }
                
            }

            for (int i = 0; i < vregs->count; i++)
                if (cfg->live_out[b].buckets[i] != tmp.buckets[i]) {
                    changed = true;
                    break;
                }
            
            set_copy(&cfg->live_out[b], &tmp);
            set_subtract(&tmp, &cfg->defines[b]);
            set_add(&tmp, &cfg->uses[b]);
            
            for (int i = 0; i < vregs->count; i++)
                if (cfg->live_in[b].buckets[i] != tmp.buckets[i]) {
                    changed = true;
                    break;
                }
            
            set_copy(&cfg->live_in[b], &tmp);
            set_clear(&tmp);
        }
    }
}


void pass_insert_phis(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    SlotTable *slots = &procedure->slots;

    Set stored_at_blocks, phi_blocks, working;

    size_t mark = arena_mark(arenas->scratch);

    set_create(&stored_at_blocks, arenas->scratch, cfg->count);
    set_create(&phi_blocks, arenas->scratch, cfg->count);
    set_create(&working, arenas->scratch, cfg->count);
    
    for (int slot = 0; slot < slots->count; slot++) {

        set_clear(&stored_at_blocks);
        set_clear(&phi_blocks);
        set_clear(&working);

        for (int block = 0; block < cfg->count; block++) {
            for (int j = 0; j < cfg->items[block].bytecode.count; j++) {
                Instr instr = cfg->items[block].bytecode.items[j];
                if (instr.opcode == OPCODE_STORE && instr.dest.type == OPERAND_SLOT && instr.dest.slot == slot) {
                    set_insert(&stored_at_blocks, block);
                    break;
                }
            }
        }

        set_add(&working, &stored_at_blocks);
         while (!set_empty(&working)) {
            int val = set_pop(&working);

            for (int j = 0; j < cfg->count; j++) {
                if (!set_has(&cfg->df[val], j)) continue; // add live in pruning, needs more computation

                if (!set_has(&phi_blocks, j)) {
                    set_insert(&phi_blocks, j);
                    set_insert(&working, j);
                }
            } 
        }

        while (!set_empty(&phi_blocks)) {
            int block = set_pop(&phi_blocks);

            Phi phi = { 0 };
            phi.slot = slot;
            phi.args = arena_alloc(arenas->persistent, sizeof(Operand) * cfg->items[block].predecessors.count);
            arena_array_append(arenas->persistent, cfg->items[block].phis, phi);
        }
    }
    arena_restore(arenas->scratch, mark);
}


void pass_kill_slots(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    for (int s = 0; s < procedure->slots.count; s++) {
        procedure->slots.items[s].killed = true;
    }
    for (int k = 0; k < cfg->count; k++) {
        int b = cfg->rpo_list[k];

        for (int i = 0; i < cfg->items[b].bytecode.count; i++) {
            Instr instr = cfg->items[b].bytecode.items[i];

            if (instr.dest.type == OPERAND_SLOT) {
                procedure->slots.items[instr.dest.slot].killed = false;
            }
            if (instr.arg1.type == OPERAND_SLOT) {
                procedure->slots.items[instr.arg1.slot].killed = false;
            }
            if (instr.arg2.type == OPERAND_SLOT) {
                procedure->slots.items[instr.arg2.slot].killed = false;
            }
            
        }
    }
}

typedef DEFINE_STACK(Operand) OperandStack;
DEFINE_STACK_DEF(OperandStack, Operand);

void pass_rename_recurs(Arenas *arenas, Procedure *procedure, OperandStack stacks[], int bid) {
    CFGraph *cfg = &procedure->cfg;
    SlotTable *slots = &procedure->slots;
    BasicBlock *block = &cfg->items[bid];

    int *pushed = arena_alloc(arenas->scratch, sizeof(int) * slots->count); // taken care of by interface to this

    for (int i = 0; i < block->phis.count; i++) {
        Phi *phi = &block->phis.items[i];
        phi->dest = allocate_vreg_explicit(arenas->persistent, procedure, vregi_from_sloti(slots->items[phi->slot]));
        OperandStack_push(&stacks[phi->slot], phi->dest);
        pushed[phi->slot]++;
    }

    for (int i = 0; i < block->bytecode.count; i++) {
        Instr *instr = &block->bytecode.items[i];

        if (instr_match(instr, OPCODE_LOAD, -1, OPERAND_SLOT, OPERAND_NONE)) {
            instr->opcode = OPCODE_COPY;
            instr->arg1 = OperandStack_peek(&stacks[instr->arg1.slot], 1);
        }

        if (instr_match(instr, OPCODE_PARAM, OPERAND_SLOT, -1, -1)) {
            instr->dest = OperandStack_peek(&stacks[instr->dest.slot], 1);
        }

        if (instr_match(instr, OPCODE_STORE, OPERAND_SLOT, -1, -1)) {
            OperandStack_push(&stacks[instr->dest.slot], instr->arg1);
            pushed[instr->dest.slot]++;
            *instr = (Instr) { 0 };
        }
    }

    for (int i = 0; i < 2; i++) {
        int s = block->terminator.successor[i];
        if (s == -1) continue;

        BasicBlock *succ = &cfg->items[s];

        int idx = -1;
        for (int j = 0; j < succ->predecessors.count; j++) {
            if (succ->predecessors.items[j] == bid) {
                idx = j;
                break;
            }
        }

        for (int j = 0; j < succ->phis.count; j++) {
            Phi *p = &succ->phis.items[j];
            p->args[idx] = OperandStack_peek(&stacks[p->slot], 1);
        }
    }

    for (int i = 0; i < cfg->count; i++) {
        if (cfg->idom[i] == bid && i != bid)
            pass_rename_recurs(arenas, procedure, stacks, i);
    }

    for (int i = 0; i < slots->count; i++) {
        for (int j = 0; j < pushed[i]; j++)
            OperandStack_pop(&stacks[i]);
    }
}


void pass_rename(Arenas *arenas, Procedure *procedure) {
    SlotTable *slots = &procedure->slots;

    size_t mark = arena_mark(arenas->scratch);
    OperandStack *stacks = arena_alloc(arenas->scratch, sizeof(OperandStack) * slots->count);

    for (int i = 0; i < slots->count; i++) {
        OperandStack_set_backing(&stacks[i], arenas->scratch);
        if (slots->items[i].param) {
            OperandStack_push(&stacks[i], allocate_vreg_explicit(arenas->persistent, procedure, vregi_from_sloti(slots->items[i])));
        }
    }

    pass_rename_recurs(arenas, procedure, stacks, procedure->cfg.entry_block);
    arena_restore(arenas->scratch, mark);
}


void pass_remove_copies(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    for (int i = 0; i < cfg->count; i++) {
        for (int j = 0; j < cfg->items[i].bytecode.count; j++) {
            Instr *instr = &cfg->items[i].bytecode.items[j];

            if (instr->opcode == OPCODE_COPY) {
                for (int k = j; k < cfg->items[i].bytecode.count; k++) {
                    Instr *marked = &cfg->items[i].bytecode.items[k];
                    if (marked->arg1.type == instr->dest.type && marked->arg1.vreg == instr->dest.vreg)
                        marked->arg1 = instr->arg1;
                    if (marked->arg2.type == instr->dest.type && marked->arg2.vreg == instr->dest.vreg)
                        marked->arg2 = instr->arg1;
                }
                Instr *marked = &cfg->items[i].terminator;
                if (marked->arg1.type == instr->dest.type && marked->arg1.vreg == instr->dest.vreg)
                    marked->arg1 = instr->arg1;
                if (marked->arg2.type == instr->dest.type && marked->arg2.vreg == instr->dest.vreg)
                     marked->arg2 = instr->arg1;                
                *instr = (Instr) { 0 };
            }
        }
    }
}


void pass_compute_liveness(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    VregInfoTable *vregs = &procedure->vregs;

    for (int i = 0; i < vregs->count; i++) {
        int bend = -1;
        
        for (int j = cfg->count - 1; j >= 0; j--) {
            int b = cfg->rpo_list[j];
            bool alive = set_has(&cfg->live_in[b], i) || set_has(&cfg->defines[b], i);
            
            if (!alive) continue;
            
            if (!set_has(&cfg->live_out[b], i)) { // dies in this block
                int iend = cfg->items[b].bytecode.count; // if for doesnt for must be in terminator
                for (int k = cfg->items[b].bytecode.count - 1; k >= 0; k--) {
                    Instr instr = cfg->items[b].bytecode.items[k];
                    if (vreg_in_use(instr, i)) {
                        iend = k;
                        break;
                    }
                }
                bend = b;
                vregs->items[i].interval.iend = iend;
            }  else {
                bend = b;
                vregs->items[i].interval.iend = cfg->items[b].bytecode.count;
            }
            
            if (set_has(&cfg->defines[b], i)) {
                int istart = 0;
                for (int k = cfg->items[b].bytecode.count - 1; k >= 0; k--) {
                    Instr instr = cfg->items[b].bytecode.items[k];
                    if (vreg_defined(instr, i)) {
                        istart = k;
                        break;
                    }
                }
                vregs->items[i].interval.istart = istart;
                vregs->items[i].interval.bstart = b;
            }

            break;
        }
        vregs->items[i].interval.bend = bend;
    }

    for (int b = 0; b < cfg->count; b++) {
        for (int s = 0; s < 2; s++) { // find backedges
            int succ = cfg->items[b].terminator.successor[s];
            
            if (cfg->items[b].terminator.successor[s] == -1) continue;
            if (!dominates(cfg, succ, b)) continue; // if block -> succ, and succ dominates block, must be loop/backedge
                                                                // thus we go to the succesors other successor and that must be exit
            int exit = exit_for(cfg, succ);
            if (exit == -1) continue;

            for (int v = 0; v < vregs->count; v++) {
                if (!set_has(&cfg->live_out[b], v) || !set_has(&cfg->live_in[succ], v)) continue;
                
                int bend = vregs->items[v].interval.bend;
                
                if (bend == -1 || cfg->rpo[exit] > cfg->rpo[bend]) {
                    vregs->items[v].interval.bend = exit;
                    vregs->items[v].interval.iend = 0;
                }
            }
        }
    }
}


void pass_crosses_call(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    Set live;
    set_create(&live, arenas->scratch, procedure->vregs.count);
    
    for (int v = 0; v < procedure->vregs.count; v++) {
        procedure->vregs.items[v].crosses_call = false;
    }
    
    for (int k = 0; k < cfg->count; k++) {
        int b = cfg->rpo_list[k];

        set_copy(&live, &cfg->live_out[b]);

        
        Instr terminator = cfg->items[b].terminator;
        int in_use[16];
        int count = vreg_if_use(terminator, in_use);
        for (int i = 0; i < count; i++) {
            set_insert(&live, in_use[i]);   
        }

        for (int i = cfg->items[b].bytecode.count - 1; i >= 0; i--) {
            Instr instr = cfg->items[b].bytecode.items[i];
            if (instr.opcode == OPCODE_CALL) {
                for (int v = 0; v < procedure->vregs.count; v++) {
                    if (set_has(&live, v) && !(instr.dest.type == OPERAND_VREG && instr.dest.vreg == v)) {
                        procedure->vregs.items[v].crosses_call = true;
                    }
                }
            }

            
            if (instr.dest.type == OPERAND_VREG) {
                set_remove(&live, instr.dest.vreg);
            }

            int in_use[16];
            int count = vreg_if_use(instr, in_use);
            for (int i = 0; i < count; i++) {
                set_insert(&live, in_use[i]);   
            }
        }
    }
}

typedef DEFINE_STACK(Color) ColorStack;

void ColorStack_set_backing(ColorStack *stack, Arena *arena) {
    stack->arena = arena;
}

void ColorStack_push(ColorStack *stack, Color value) {
    if (stack->cursor + 1 > stack->capacity) {
        size_t old = stack->capacity;
        size_t new = old == 0 ? 16 : old * 2;
        stack->stack = arena_realloc(stack->arena, stack->stack, old * sizeof(Color), new * sizeof(Color)); 
        stack->capacity = new;
    }
    stack->stack[stack->cursor++] = value;
}

Color ColorStack_pop(ColorStack *stack, PhysRegFlags filter) {
    bool found = false;
    Color popped;
    for (int start = stack->cursor - 1; start >= 0; start--) {
        if ((stack->stack[start].flags & filter) == filter) {
            popped = stack->stack[start];
            found = true;

            for (size_t move = start + 1; move < stack->cursor; move++) {
                stack->stack[move - 1] = stack->stack[move];
            }
            stack->cursor--;
            
            break;
        }
    }
    
    return found ? popped : (Color) { .index = -1 };
}

void pass_color_cfg_recurs(Arenas *arenas, Procedure *procedure, int block, Set *live, ColorStack *free) {
    CFGraph *cfg = &procedure->cfg;
    VregInfoTable *vregs = &procedure->vregs;
    BasicBlock *blk = &cfg->items[block];

    size_t mark = arena_mark(arenas->scratch);
    size_t free_mark = free->cursor;
    Set tmp;
    set_create(&tmp, arenas->scratch, vregs->count);
    set_copy(&tmp, live);

    for (int p = 0; p < blk->phis.count; p++) {
        Phi phi = blk->phis.items[p];
        set_insert(live, phi.dest.vreg);

        PhysRegFlags filter = procedure->vregs.items[phi.dest.vreg].crosses_call ?
                (PR_GENERAL_PURPOSE | PR_CALLEE_SAVED) : PR_GENERAL_PURPOSE;
            
        if (free->cursor > 0) vregs->items[phi.dest.vreg].color = ColorStack_pop(free, filter);
        else assert(0 && "no spills yet...");
    }

    for (int i = 0; i < blk->bytecode.count; i++) {
        for (int v = 0; v < vregs->count; v++) {
            if (set_has(live, v)
                && !set_has(&cfg->live_out[block], v)
                && vregs->items[v].interval.iend <= i
                && vregs->items[v].interval.bend == block
            ) {
                set_remove(live, v);
                ColorStack_push(free, vregs->items[v].color);
            }
        }

        Instr instr = blk->bytecode.items[i];
        if (instr.dest.type == OPERAND_VREG) {
            set_insert(live, instr.dest.vreg);


            PhysRegFlags filter = PR_GENERAL_PURPOSE;
            if (vregs->items[instr.dest.vreg].crosses_call) {
                filter |= PR_CALLEE_SAVED;
            }

            if (free->cursor > 0) vregs->items[instr.dest.vreg].color = ColorStack_pop(free, filter);
            else assert(0 && "no spills yet...");            


            if ((vregs->items[instr.dest.vreg].color.flags & PR_CALLEE_SAVED) == PR_CALLEE_SAVED) {
                set_insert(&procedure->saved_colors, vregs->items[instr.dest.vreg].color.index);
            }
            
        }
    }

    for (int b = 0; b < cfg->count; b++) {
        if (cfg->idom[b] == block && b != block) pass_color_cfg_recurs(arenas, procedure, b, live, free);
    }

    set_copy(live, &tmp);
    arena_restore(arenas->scratch, mark);
    free->cursor = free_mark;
}


void pass_color_cfg(Arenas *arenas, Procedure *procedure) {
    size_t mark = arena_mark(arenas->scratch);
    
    Set live;
    set_create(&live, arenas->scratch, procedure->vregs.count);
    set_create(&procedure->saved_colors, arenas->persistent, 16);

    ColorStack stack = { 0 };
    ColorStack_set_backing(&stack, arenas->scratch);

    for (int i = 11; i > 7; i--) {
        ColorStack_push(&stack, (Color) { .index = i, .flags = PR_GENERAL_PURPOSE | PR_CALLEE_SAVED });
    }    
    for (int i = 7; i >= 0; i--) {
        ColorStack_push(&stack, (Color) { .index = i, .flags = PR_GENERAL_PURPOSE});
    }
    
    pass_crosses_call(arenas, procedure);
    pass_color_cfg_recurs(arenas, procedure, procedure->cfg.entry_block, &live, &stack);
    arena_restore(arenas->scratch, mark);
}


void pass_phis_into_copies(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    for (int b = 0; b < cfg->count; b++) {
        for (int p = 0; p < cfg->items[b].phis.count; p++) { // arg should be same as pred
            Phi phi = cfg->items[b].phis.items[p];
            for (int a = 0; a < cfg->items[b].predecessors.count; a++) {
                Instr instr = { 0 };
                instr.opcode = OPCODE_COPY;
                instr.dest = phi.dest;
                instr.arg1 = phi.args[a];
                emit_instr_into_block(arenas->persistent, &cfg->items[cfg->items[b].predecessors.items[a]], instr);
            }
        }
    }
}


void pass_sweep_nops(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;
    for (int b = 0; b < cfg->count; b++) {
        Bytecode new;
        new.items = arena_alloc(arenas->persistent, sizeof(Instr) * cfg->items[b].bytecode.count);
        new.capacity = cfg->items[b].bytecode.count;
        new.count = 0;
        for (int i = 0; i < cfg->items[b].bytecode.count; i++) {
            if (cfg->items[b].bytecode.items[i].opcode != OPCODE_NONE) {
                new.items[new.count++] = cfg->items[b].bytecode.items[i];
            }
        }
        cfg->items[b].bytecode = new;
    }
}


const Opcode op_cond_opcode_table[OPCODE_COUNT] = {
    [OPCODE_JMP] = OPCODE_JMP,
    [OPCODE_BNE] = OPCODE_BEQ,
    [OPCODE_BEQ] = OPCODE_BNE,
    [OPCODE_BGE] = OPCODE_BLT,
    [OPCODE_BLE] = OPCODE_BGT,
    [OPCODE_BGT] = OPCODE_BLE,
    [OPCODE_BLT] = OPCODE_BGE,
};


void pass_three_op_to_two(Arenas *arenas, Procedure *procedure) {
    CFGraph *cfg = &procedure->cfg;

    for (int b = 0; b < cfg->count; b++) {
        int estimated_size = cfg->items[b].bytecode.count * 2;
        Bytecode bc = { 0 };
        bc.capacity = estimated_size;
        bc.items = arena_alloc(arenas->persistent, estimated_size * sizeof(Instr));

        for (int i = 0; i < cfg->items[b].bytecode.count; i++) {
            Instr instr = cfg->items[b].bytecode.items[i];
            switch (instr.opcode) {
                case OPCODE_ADD:
                case OPCODE_SUB:
                case OPCODE_MUL: {
                    Instr copy = { 0 };
                    copy.opcode = OPCODE_COPY;
                    copy.dest = allocate_vreg_explicit(arenas->persistent, procedure, procedure->vregs.items[instr.arg1.vreg]);
                    copy.arg1 = instr.arg1;
                    instr.arg1 = copy.dest;
                    bc.items[bc.count++] = copy;
                    bc.items[bc.count++] = instr;
                    break;
                }
                default:
                    bc.items[bc.count++] = instr;
                    break;
            }
        }
        cfg->items[b].bytecode = bc;
    }
}
