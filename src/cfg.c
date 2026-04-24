
#include "common.h"
#include "ir.h"

 int intersection(int *dominates, int *rpo, int left, int right) {
     while (left != right) {
         while (rpo[left] < rpo[right]) right = dominates[right];
         while (rpo[left] > rpo[right]) left = dominates[left];
     }
     return right;
 }


void fill_rpo(CFGraph *cfg, int *fill, int *index, int block) {
    if (block == -1 || fill[block] != -1) return;

    fill[block] = 0;
    
    fill_rpo(cfg, fill, index, cfg->items[block].terminator.successor[0]);
    fill_rpo(cfg, fill, index, cfg->items[block].terminator.successor[1]);

    (*index)++;
    fill[block] = cfg->count - *index;
}


void dominance(Arena *arena, CFGraph *cfg) {
    int *dominates = arena_alloc(arena, cfg->count * sizeof(int));
    int *rpo = arena_alloc(arena, cfg->count * sizeof(int));
    memset(dominates, -1, cfg->count * sizeof(int));
    memset(rpo, -1, cfg->count * sizeof(int));

    int i = 0;
    fill_rpo(cfg, rpo, &i, cfg->entry_block);

    int *iter = arena_alloc(arena, cfg->count * sizeof(int));
    for (int i = 0; i < cfg->count; i++) {
      if (rpo[i] != -1) iter[rpo[i]] = i;
    }

    dominates[cfg->entry_block] = cfg->entry_block;

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < cfg->count; i++) {
            int curr = iter[i];

            int mdom = -1;
            for (int k = 0; k < cfg->items[curr].predecessors.count; k++) {
                int p = cfg->items[curr].predecessors.items[k];
                if (dominates[p] == -1) continue;
                mdom = (mdom == -1) ? p :  intersection(dominates, rpo, mdom, p);
            }
            
            if (mdom != -1 && dominates[curr] != mdom) {
                dominates[curr] = mdom;
                changed = true;
            }

        }
    }

    cfg->dominance = dominates;
    cfg->rpo = rpo;
    
    // printf("dom_graph = {\n");
    // for (int i = 0; i < cfg->count; i++) {
    //     printf("dom(%d) = {", i);
    //     int curr = i;
    //     while (curr != cfg->entry_block) {
    //         printf("%d, ", curr);
    //         curr = dominates[curr];
    //     }
    //     printf("%d", curr);
    //     printf("},\n");
    // }
    // printf("}");
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


void rename_block(EmitContext *context, Procedure *procedure, int block, OperandStack stacks[procedure->slots.count]) {
    int pushed[procedure->slots.count];
    memset(pushed, 0, sizeof(int) * procedure->slots.count);
    
    for (int i = 0; i < procedure->cfg.items[block].phis.count; i++) {
        Phi *phi = &procedure->cfg.items[block].phis.items[i];
        phi->dest = allocate_vreg_explicit(context, (VregInfo) { .size = procedure->slots.items[phi->slot].size, .sign = procedure->slots.items[phi->slot].sign});
        OperandStack_push(&stacks[phi->slot], phi->dest);

        pushed[phi->slot]++;
    }

    for (int i = 0; i < procedure->cfg.items[block].bytecode.count; i++) {
        Instr *instr = &procedure->cfg.items[block].bytecode.items[i];

        if (instr->opcode == OPCODE_LOAD && instr->arg1.type == OPERAND_SLOT) {
            instr->opcode = OPCODE_COPY;
            instr->arg1 = OperandStack_peek(&stacks[instr->arg1.slot], 1);
        }

        if (instr->opcode == OPCODE_PARAM && instr->dest.type == OPERAND_SLOT) {
            instr->dest = OperandStack_peek(&stacks[instr->dest.slot], 1);
        }
        
        if (instr->opcode == OPCODE_STORE && instr->dest.type == OPERAND_SLOT) {
            OperandStack_push(&stacks[instr->dest.slot], instr->arg1);
            pushed[instr->dest.slot]++;
            instr->opcode = OPCODE_NONE;
        }
        
    }

    for (int i = 0; i < 2; i++) {
        int succ = procedure->cfg.items[block].terminator.successor[i];

        if (succ == -1) continue;


        int idx = -1;
        for (int j = 0; j < procedure->cfg.items[succ].predecessors.count; j++) {
            if (procedure->cfg.items[succ].predecessors.items[j] == block) {
                idx = j;
                break;
            }
        }

        for (int j = 0; j < procedure->cfg.items[succ].phis.count; j++) {
            Phi *p = &procedure->cfg.items[succ].phis.items[j];
            p->args[idx] = OperandStack_peek(&stacks[p->slot], 1);
        }
    }

    for (int i = 0; i < procedure->cfg.count; i++) {
        if (procedure->cfg.dominance[i] == block && i != block)
            rename_block(context, procedure, i, stacks);
    }

    for (int i = 0; i < procedure->slots.count; i++) {
        for (int j = 0; j < pushed[i]; j++)
            OperandStack_pop(&stacks[i]);
    }
}


void insert_phis(Arena *arena, Procedure *procedure) {
    Set *DF = arena_alloc(arena, sizeof(Set) * procedure->cfg.count);

    for (int i = 0; i < procedure->cfg.count; i++) {
        set_create(&DF[i], arena, procedure->cfg.count);
    }

    for (int i = 0; i < procedure->cfg.count; i++) {
        if (procedure->cfg.items[i].predecessors.count < 2) continue;
        
        for (int j = 0; j < procedure->cfg.items[i].predecessors.count; j++) {
            int runner = procedure->cfg.items[i].predecessors.items[j];
            
            while (runner != procedure->cfg.dominance[i]) {
                set_insert(&DF[runner], i);
                runner = procedure->cfg.dominance[runner];
            }
            
        }
    }

    for (int slot = 0; slot < procedure->slots.count; slot++) {

        Set stored_at_blocks, phi_blocks, working;

        set_create(&stored_at_blocks, arena, procedure->cfg.count);
        set_create(&phi_blocks, arena, procedure->cfg.count);
        set_create(&working, arena, procedure->cfg.count);

        for (int block = 0; block < procedure->cfg.count; block++) {
            for (int j = 0; j < procedure->cfg.items[block].bytecode.count; j++) {
                Instr instr = procedure->cfg.items[block].bytecode.items[j];
                if (instr.opcode == OPCODE_STORE && instr.dest.type == OPERAND_SLOT && instr.dest.slot == slot) {
                    set_insert(&stored_at_blocks, block);
                    break;
                }
            }
        }

        set_add(&working, &stored_at_blocks);
         while (!set_empty(&working)) {
            int val = set_pop(&working);

            for (int j = 0; j < procedure->cfg.count; j++) {
                if (!set_has(&DF[val], j)) continue;

                if (!set_has(&phi_blocks, j)) {
                    set_insert(&phi_blocks, j);
                    set_insert(&working, j);
                }
            } 
        }

        
        while (!set_empty(&phi_blocks)) {
            int block = set_pop(&phi_blocks);

            Phi phi = (Phi) {
                .slot = slot,
                .args = arena_alloc(arena, sizeof(Operand) * procedure->cfg.items[block].predecessors.count)
            };

            arena_array_append(arena, procedure->cfg.items[block].phis, phi);
        }
    }     
}


void remove_copies(Procedure *procedure) {
    for (int i = 0; i < procedure->cfg.count; i++) {
        for (int j = 0; j < procedure->cfg.items[i].bytecode.count; j++) {
            Instr *instr = &procedure->cfg.items[i].bytecode.items[j];

            if (instr->opcode == OPCODE_COPY) {
                instr->opcode = OPCODE_NONE;
                for (int k = j; k < procedure->cfg.items[i].bytecode.count; k++) {
                    Instr *marked = &procedure->cfg.items[i].bytecode.items[k];
                    if (marked->arg1.type == instr->dest.type && marked->arg1.vreg == instr->dest.vreg)
                        marked->arg1 = instr->arg1;
                    if (marked->arg2.type == instr->dest.type && marked->arg2.vreg == instr->dest.vreg)
                        marked->arg2 = instr->arg1;
                }
                Instr *marked = &procedure->cfg.items[i].terminator;
                if (marked->arg1.type == instr->dest.type && marked->arg1.vreg == instr->dest.vreg)
                    marked->arg1 = instr->arg1;
                if (marked->arg2.type == instr->dest.type && marked->arg2.vreg == instr->dest.vreg)
                     marked->arg2 = instr->arg1;                
            }
        }
    }
}


