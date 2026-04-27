
#include "ast.h"
#include "common.h"
#include "ir.h"

void passes_preparation(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    cfg->idom = arena_alloc(context->arena, sizeof(int) * cfg->count);
    cfg->rpo = arena_alloc(context->arena, sizeof(int) * cfg->count);
    cfg->rpo_list = arena_alloc(context->arena, sizeof(int) * cfg->count);
    cfg->live_in = arena_alloc(context->arena, sizeof(Set) * cfg->count),
    cfg->live_out = arena_alloc(context->arena, sizeof(Set) * cfg->count),
    cfg->uses = arena_alloc(context->arena, sizeof(Set) * cfg->count),
    cfg->defines = arena_alloc(context->arena, sizeof(Set) * cfg->count);
    cfg->df = arena_alloc(context->arena, sizeof(Set) * cfg->count);
    cfg->global_pos = arena_alloc(context->arena, sizeof(Set) * cfg->count);
    
    for (int i = 0; i < cfg->count; i++) {
        set_create(&cfg->df[i], context->arena, cfg->count);
    }
    
    memset(cfg->idom, -1, sizeof(int) * cfg->count);
    memset(cfg->rpo, -1, sizeof(int) * cfg->count);
    memset(cfg->rpo_list, -1, sizeof(int) * cfg->count);
   
    int i = 0;
    fill_rpo(cfg, &i, context->procedure->cfg.entry_block);
    fill_predecessors(cfg);
    fill_idom(cfg); // needs rpo, rpo_list, predecessors
    fill_df(cfg);
    fill_liveness(context); // needs vreg info
}


bool vreg_defined(Instr instr, int vreg) {
    return (instr.dest.type == OPERAND_VREG && instr.dest.vreg == vreg);
}

bool vreg_in_use(Instr instr, int vreg) {
    return (instr.arg1.type == OPERAND_VREG && instr.arg1.vreg == vreg)
           || (instr.arg2.type == OPERAND_VREG && instr.arg2.vreg == vreg)
           || (instr.dest.type == OPERAND_MEM && instr.dest.mem.base.type == ADDR_VREG && instr.dest.mem.base.vreg == vreg)
           || (instr.dest.type == OPERAND_MEM && instr.dest.mem.index.type == ADDR_VREG && instr.dest.mem.index.vreg == vreg);
}


int vreg_if_use(Instr instr, int *vregs) {
    int size = 0;
    if (instr.arg1.type == OPERAND_VREG) {
        vregs[size++] = instr.arg1.vreg;
    }
    if (instr.arg2.type == OPERAND_VREG) {
        vregs[size++] = instr.arg2.vreg;
    }
    if (instr.dest.type == OPERAND_MEM) {
        if (instr.dest.mem.base.type == ADDR_VREG) {
            vregs[size++] = instr.dest.mem.base.vreg;
        }
        if (instr.dest.mem.index.type == ADDR_VREG) {
            vregs[size++] = instr.dest.mem.index.vreg;
        }
    }
    return size;
}


bool dominates(CFGraph *cfg, int a, int b) { // does A dominate B
    while (b != cfg->entry_block) {
        if (a == cfg->idom[b]) return true;
        b = cfg->idom[b];
    }
    return a == cfg->entry_block;
}


int exit_for(CFGraph *cfg, int a) {
    int succ1 = cfg->items[a].terminator.successor[0];
    int succ2 = cfg->items[a].terminator.successor[1];
    if (succ1 == -1) return -1;
    return cfg->rpo[succ1] > cfg->rpo[succ2] ? succ1 : succ2;
}


void fill_rpo(CFGraph *cfg, int *index, int block) {
    if (block == -1 || cfg->rpo[block] != -1) return;

    cfg->rpo[block] = 0;
    
    fill_rpo(cfg, index, cfg->items[block].terminator.successor[1]);
    fill_rpo(cfg, index, cfg->items[block].terminator.successor[0]);

    (*index)++;
    cfg->rpo[block] = cfg->count - *index;
    cfg->rpo_list[cfg->count - *index] = block;
}


int intersection(int *dominates, int *rpo, int left, int right) {
     while (left != right) {
         while (rpo[left] < rpo[right]) right = dominates[right];
         while (rpo[left] > rpo[right]) left = dominates[left];
     }
     return right;
}


void fill_idom(CFGraph *cfg) {
    cfg->idom[cfg->entry_block] = cfg->entry_block;

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < cfg->count; i++) {
            int curr = cfg->rpo_list[i];
            int mdom = -1;
            for (int k = 0; k < cfg->items[curr].predecessors.count; k++) {
                int p = cfg->items[curr].predecessors.items[k];
                if (cfg->idom[p] == -1) continue;
                mdom = (mdom == -1) ? p :  intersection(cfg->idom, cfg->rpo, mdom, p);
            }
            if (mdom != -1 && cfg->idom[curr] != mdom) {
                cfg->idom[curr] = mdom;
                changed = true;
            }
        }
    }
}


void add_predecessor(CFGraph *cfg, int block, int with) {
    if (block == -1) return;

    bool add = (with != -1);
    for (int i = 0; i < cfg->items[block].predecessors.count; i++) {
        if (cfg->items[block].predecessors.items[i] == with) {
            return;
        }
    }
    if (add) array_append(cfg->items[block].predecessors, with);

    add_predecessor(cfg, cfg->items[block].terminator.successor[0], block);
    add_predecessor(cfg, cfg->items[block].terminator.successor[1], block);
}


void fill_predecessors(CFGraph *cfg) {
    add_predecessor(cfg, cfg->entry_block, -1);
}


void fill_df(CFGraph *cfg) {
    for (int i = 0; i < cfg->count; i++) {
        if (cfg->items[i].predecessors.count < 2) continue;
        
        for (int j = 0; j < cfg->items[i].predecessors.count; j++) {
            int runner = cfg->items[i].predecessors.items[j];
            
            while (runner != cfg->idom[i]) {
                set_insert(&cfg->df[runner], i);
                runner = cfg->idom[runner];
            }
            
        }
    }
}


void fill_liveness(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    VregInfoTable *vregs = &context->procedure->vregs;

    for (int i = 0; i < cfg->count; i++) {
        set_create(&cfg->live_in[i], context->arena, vregs->count);
        set_create(&cfg->live_out[i], context->arena, vregs->count);
        set_create(&cfg->uses[i], context->arena, vregs->count);
        set_create(&cfg->defines[i], context->arena, vregs->count);
    }

    Set tmp;
    set_create(&tmp, context->scratch, vregs->count);
    
    for (int b = 0; b < cfg->count; b++) {
        BasicBlock *block = &cfg->items[b];
        Instr instr;

        // for (int i = 0; i < block->phis.count; i++) {
        //     Phi phi = block->phis.items[i];
        //     if (phi.dest.type == OPERAND_VREG) {
        //         set_insert(&cfg->defines[b], phi.dest.vreg);
        //     }

        //     for (int j = 0; j < block->predecessors.count; j++) {
        //         int pred = block->predecessors.items[j];
        //         if (phi.args[j].type == OPERAND_VREG && !set_has(&cfg->defines[b], phi.args[j].vreg))
        //             set_insert(&cfg->uses[pred], phi.args[j].vreg);
        //     }
        // }

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



void pass_insert_phis(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    SlotTable *slots = &context->procedure->slots;

    Set stored_at_blocks, phi_blocks, working;

    size_t mark = arena_mark(context->scratch);

    set_create(&stored_at_blocks, context->scratch, cfg->count);
    set_create(&phi_blocks, context->scratch, cfg->count);
    set_create(&working, context->scratch, cfg->count);
    
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
            phi.args = arena_alloc(context->arena, sizeof(Operand) * cfg->items[block].predecessors.count);
            arena_array_append(context->arena, cfg->items[block].phis, phi);
        }
    }
    arena_restore(context->scratch, mark);
}


inline VregInfo vregi_from_sloti(SlotInfo slot) {
    return (VregInfo) {
        .size = slot.size,
        .sign = slot.sign
    };
}


void pass_rename(EmitContext *context) {
    SlotTable *slots = &context->procedure->slots;

    size_t mark = arena_mark(context->scratch);
    OperandStack *stacks = arena_alloc(context->scratch, sizeof(OperandStack) * slots->count);

    for (int i = 0; i < slots->count; i++) {
        OperandStack_set_backing(&stacks[i], context->scratch);
        if (slots->items[i].param) {
            OperandStack_push(&stacks[i], allocate_vreg_explicit(context, vregi_from_sloti(slots->items[i])));
        }
    }

    pass_rename_recurs(context, stacks, context->procedure->cfg.entry_block);
    arena_restore(context->scratch, mark);
}


void pass_kill_slots(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    for (int s = 0; s < context->procedure->slots.count; s++) {
        context->procedure->slots.items[s].killed = true;
    }
    for (int k = 0; k < cfg->count; k++) {
        int b = cfg->rpo_list[k];

        for (int i = 0; i < cfg->items[b].bytecode.count; i++) {
            Instr instr = cfg->items[b].bytecode.items[i];

            if (instr.dest.type == OPERAND_SLOT) {
                context->procedure->slots.items[instr.dest.slot].killed = false;
            }
            if (instr.arg1.type == OPERAND_SLOT) {
                context->procedure->slots.items[instr.arg1.slot].killed = false;
            }
            if (instr.arg2.type == OPERAND_SLOT) {
                context->procedure->slots.items[instr.arg2.slot].killed = false;
            }
            
        }
    }
}


// if arg is -1, matches anything
inline bool instr_match(Instr *instr, Opcode opcode, OperandType dest, OperandType arg1, OperandType arg2) {
    return (instr->opcode == opcode &&
                (dest == -1 || instr->dest.type == dest) &&
                (arg1 == -1 || instr->arg1.type == arg1) &&
                (arg2 == -1 || instr->arg2.type == arg2));
}


void pass_rename_recurs(EmitContext *context, OperandStack stacks[], int bid) {
    CFGraph *cfg = &context->procedure->cfg;
    SlotTable *slots = &context->procedure->slots;
    BasicBlock *block = &cfg->items[bid];

    int *pushed = arena_alloc(context->scratch, sizeof(int) * slots->count); // taken care of by interface to this

    for (int i = 0; i < block->phis.count; i++) {
        Phi *phi = &block->phis.items[i];
        phi->dest = allocate_vreg_explicit(context, vregi_from_sloti(slots->items[phi->slot]));
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
            pass_rename_recurs(context, stacks, i);
    }

    for (int i = 0; i < slots->count; i++) {
        for (int j = 0; j < pushed[i]; j++)
            OperandStack_pop(&stacks[i]);
    }
}


void pass_remove_copies(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
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


void pass_compute_liveness(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    VregInfoTable *vregs = &context->procedure->vregs;

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


void pass_crosses_call(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    Set live;
    set_create(&live, context->scratch, context->procedure->vregs.count);
    
    for (int v = 0; v < context->procedure->vregs.count; v++) {
        context->procedure->vregs.items[v].crosses_call = false;
    }
    
    for (int k = 0; k < cfg->count; k++) {
        int b = cfg->rpo_list[k];

        set_copy(&live, &cfg->live_out[b]);

        for (int i = cfg->items[b].bytecode.count - 1; i >= 0; i--) {
            Instr instr = cfg->items[b].bytecode.items[i];
            if (instr.opcode == OPCODE_CALL) {
                for (int v = 0; v < context->procedure->vregs.count; v++) {
                    if (set_has(&live, v) && !(instr.dest.type == OPERAND_VREG && instr.dest.vreg == v)) {
                        context->procedure->vregs.items[v].crosses_call = true;
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


void pass_color_cfg(EmitContext *context) {
    size_t mark = arena_mark(context->scratch);
    
    Set live;
    set_create(&live, context->scratch, context->procedure->vregs.count);
    set_create(&context->procedure->saved_colors, context->arena, 16);

    ColorStack stack = { 0 };
    ColorStack_set_backing(&stack, context->scratch);

    for (int i = 11; i > 7; i--) {
        ColorStack_push(&stack, (Color) { .index = i, .flags = PR_GENERAL_PURPOSE | PR_CALLEE_SAVED });
    }    
    for (int i = 7; i >= 0; i--) {
        ColorStack_push(&stack, (Color) { .index = i, .flags = PR_GENERAL_PURPOSE});
    }
    
    pass_crosses_call(context);
    pass_color_cfg_recurs(context, context->procedure->cfg.entry_block, &live, &stack);
    arena_restore(context->scratch, mark);
}


void pass_color_cfg_recurs(EmitContext *context, int block, Set *live, ColorStack *free) {
    CFGraph *cfg = &context->procedure->cfg;
    VregInfoTable *vregs = &context->procedure->vregs;
    BasicBlock *blk = &cfg->items[block];

    size_t mark = arena_mark(context->scratch);
    size_t free_mark = free->cursor;
    Set tmp;
    set_create(&tmp, context->scratch, vregs->count);
    set_copy(&tmp, live);

    for (int p = 0; p < blk->phis.count; p++) {
        Phi phi = blk->phis.items[p];
        set_insert(live, phi.dest.vreg);

        PhysRegFlags filter = context->procedure->vregs.items[phi.dest.vreg].crosses_call ?
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
                set_insert(&context->procedure->saved_colors, vregs->items[instr.dest.vreg].color.index);
            }
            
        }
    }

    for (int b = 0; b < cfg->count; b++) {
        if (cfg->idom[b] == block && b != block) pass_color_cfg_recurs(context, b, live, free);
    }

    set_copy(live, &tmp);
    arena_restore(context->scratch, mark);
    free->cursor = free_mark;
}


void pass_phis_into_copies(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    for (int b = 0; b < cfg->count; b++) {
        for (int p = 0; p < cfg->items[b].phis.count; p++) { // arg should be same as pred
            Phi phi = cfg->items[b].phis.items[p];
            for (int a = 0; a < cfg->items[b].predecessors.count; a++) {
                context->current_block = cfg->items[b].predecessors.items[a];
                emit_instr(context, OPCODE_COPY, 2, phi.dest, phi.args[a]);
            }
        }
    }
}


void pass_sweep_nops(EmitContext *context) {
    CFGraph *cfg = &context->procedure->cfg;
    for (int b = 0; b < cfg->count; b++) {
        Bytecode new;
        new.items = arena_alloc(context->arena, sizeof(Instr) * cfg->items[b].bytecode.count);
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


void pass_three_op_to_two(EmitContext* context) {
    CFGraph *cfg = &context->procedure->cfg;

    for (int b = 0; b < cfg->count; b++) {
        int estimated_size = cfg->items[b].bytecode.count * 2;
        Bytecode bc = { 0 };
        bc.capacity = estimated_size;
        bc.items = arena_alloc(context->arena, estimated_size * sizeof(Instr));

        for (int i = 0; i < cfg->items[b].bytecode.count; i++) {
            Instr instr = cfg->items[b].bytecode.items[i];
            switch (instr.opcode) {
                case OPCODE_ADD:
                case OPCODE_SUB:
                case OPCODE_MUL: {
                    Instr copy = { 0 };
                    copy.opcode = OPCODE_COPY;
                    copy.dest = allocate_vreg_explicit(context, context->procedure->vregs.items[instr.arg1.vreg]);
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


Bytecode pass_flatten(EmitContext* context) {
    CFGraph *cfg = &context->procedure->cfg;
    
    int size = 0;
    for (int b = 0; b < cfg->count; b++) {
        size += cfg->items[b].bytecode.count + 1; // + 1 for terminator every block has one
    }

    Bytecode bytecode = { 0 };
    bytecode.capacity = size;
    bytecode.items = arena_alloc(context->arena, size * sizeof(Instr));


    for (int k = 0; k < cfg->count; k++) {
        int b = cfg->rpo_list[k];

        cfg->global_pos[b] = bytecode.count;
        for (int i = 0; i < cfg->items[b].bytecode.count; i++) {
            bytecode.items[bytecode.count++] = cfg->items[b].bytecode.items[i];
        }

        Instr terminator = cfg->items[b].terminator;
        if (terminator.opcode == OPCODE_JMP && cfg->idom[terminator.successor[0]] == b) continue;

        if (terminator.opcode == OPCODE_JMP) {
            terminator.dest.type = OPERAND_BLOCK;
            terminator.dest.block = terminator.successor[0];
        } else if (terminator.opcode == OPCODE_RET) {
            
        } else {
            terminator.dest.type = OPERAND_BLOCK;
            terminator.dest.block = terminator.successor[1];
            terminator.opcode = op_cond_opcode_table[terminator.opcode];
        }

        bytecode.items[bytecode.count++] = terminator;
    }

    // for (int i = 0; i < bytecode.count; i++) {
    //     Instr *instr = &bytecode.items[i];
    //     if (instr->opcode == OPCODE_JMP) {
    //     } else if (opcode_is_branch(instr->opcode)) {
            
    //     }
    // }

    return bytecode;
}


