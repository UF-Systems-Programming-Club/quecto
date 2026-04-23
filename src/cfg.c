
#include "ir.h"

 int intersection(int *dominates, int *rpo, int left, int right) {
     while (left != right) {
         while (rpo[left] < rpo[right]) right = dominates[right];
         while (rpo[left] > rpo[right]) left = dominates[left];
         if (left == -1 || right == -1)
             return -1;
     }
     return right;
 }


void fill_rpo(CFGraph *cfg, int *fill, int *index, int block) {
    if (block == -1 || fill[block] != -1) return;
    
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

    int *stack = arena_alloc(arena, cfg->count * sizeof(int));
    int cursor = 0;
    stack[cursor++] = cfg->entry_block;
   
    while (cursor > 0) {
        int curr = stack[--cursor];
        
        if (dominates[curr] != -1) continue;
        
        while (cursor > 0 && stack[cursor - 1] == curr) { cursor--; }

        if (cfg->items[curr].predecessors.count == 0) dominates[curr] = curr;
        if (cfg->items[curr].predecessors.count == 1) dominates[curr] = cfg->items[curr].predecessors.items[0];
        if (cfg->items[curr].predecessors.count > 1) {
            int mdom = -1;
            for (int i = 1; i < cfg->items[curr].predecessors.count; i++) {
                int new = intersection(dominates, rpo, cfg->items[curr].predecessors.items[0], cfg->items[curr].predecessors.items[i]);
                if (mdom == -1 || rpo[new] < rpo[mdom]) mdom = new;
            }
            dominates[curr] = mdom;
        }

        int succ1 = cfg->items[curr].terminator.successor[0];
        int succ2 = cfg->items[curr].terminator.successor[1];

        if (succ1 != -1) stack[cursor++] = succ1;
        if (succ2 != -1) stack[cursor++] = succ2;
    }

    cfg->dominance = dominates;
    cfg->rpo = rpo;
    
    printf("dom_graph = {\n");
    for (int i = 0; i < cfg->count; i++) {
        printf("dom(%d) = {", i);
        int curr = i;
        while (curr != cfg->entry_block) {
            printf("%d, ", curr);
            curr = dominates[curr];
        }
        printf("%d", curr);
        printf("},\n");
    }
    printf("}");
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
