#ifndef PASSES_H
#define PASSES_H

#include "ir.h"
#include "emission.h"

typedef void (*PassFn)(Arenas *arena, Procedure *procedure);
typedef PassFn* Passes; // should be NULL-terminated

void passes_run(Program *program, Passes passes, Arenas arenas);
void passes_preparation(Arenas *arenas, Procedure *procedure);

void passes_preparation(Arenas *arena, Procedure *procedure);
void pass_liveness(Arenas *arena, Procedure *procedure);
void pass_insert_phis(Arenas *arena, Procedure *procedure);
void pass_rename(Arenas *arena, Procedure *procedure);
void pass_compute_liveness(Arenas *arena, Procedure *procedure);
void pass_remove_copies(Arenas *arena, Procedure *procedure);
void pass_color_cfg(Arenas *arena, Procedure *procedure);
void pass_crosses_call(Arenas *arenas, Procedure *procedure);
void pass_precoloring(Arenas *arena, Procedure *procedure);
void pass_sweep_nops(Arenas *arena, Procedure *procedure);
void pass_phis_into_copies(Arenas *arena, Procedure *procedure);
void pass_three_op_to_two(Arenas *arena, Procedure *procedure);
void pass_kill_slots(Arenas *arena, Procedure *procedure);

void debug_pass_print(Arenas *_, Procedure *procedure);
void debug_pass_print_colored(Arenas *_, Procedure *procedure);

#endif 
