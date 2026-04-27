// #include <stdio.h>

#include "ir.h"

// IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode, int vreg_count) {
//     IntervalArray intervals;
//     array_init(intervals, vreg_count);

//     for (int vreg = 0; vreg < vreg_count; vreg++) {
//         Interval interval = {0};
//         interval.vreg = vreg;
//         // find start
//         for (int i = 0; i < bytecode.count; i++) {
//             if (bytecode.items[i].dest.type == OPERAND_VREG && bytecode.items[i].dest.vreg == vreg) {
//                 interval.start = i;
//                 break;
//             }
//         }

//         // find end
//         for (int i = bytecode.count - 1; i >= 0; i--) {
//             if (bytecode.items[i].arg1.type == OPERAND_VREG && bytecode.items[i].arg1.vreg == vreg) {
//                 interval.end = i;
//                 break;
//             }
//             if (bytecode.items[i].arg2.type == OPERAND_VREG && bytecode.items[i].arg2.vreg == vreg) {
//                 interval.end = i;
//                 break;
//             }
//             if (bytecode.items[i].arg1.type == OPERAND_STACK && bytecode.items[i].arg1.base == vreg) {
//                 interval.end = i;
//                 break;
//             }
//             if (bytecode.items[i].arg1.type == OPERAND_STACK && bytecode.items[i].arg1.base == vreg) {
//                 interval.end = i;
//                 break;
//             }
//         }

//         array_append(intervals, interval);
//     }

//     return intervals;
// }

// void reg_alloc(Procedure *procedure, int vreg_count) { // 4 max active
//     int inactive_count = 4;
//     int inactive[4] = { 1, 2, 3, 4 };
//     for (int i = 0; i < procedure->cfg.count; i++) {
//         int b = procedure->cfg.rpo_list[i];

//         for (int j = 0; j < procedure->cfg.items[b].bytecode.count; j++) {
//             for (int v = 0; )
//         }
//     }c
// }c


// // NOTE: assumes intervals is already sorted
// LocationArray linear_scan_register_allocation(IntervalArray *intervals, int vreg_count, PhysRegs *pregs) {
//     LocationArray locations = {0};
//     bool active_registers[] = { 1, 1, 1, 1, };
//     array_init(locations, vreg_count);
//     locations.count = vreg_count;
//     IntervalArray active = {0};

//     for (int i = 0; i < intervals->count; i++) {
//         Interval *interval = &intervals->items[i];
//         Location location = {0};
//         location.type = LOC_REGISTER;
//         int vreg = intervals->items[i].vreg;

//         // Expire old intervals
//         for (int j = 0; j < active.count; j++) {
//             if (active.items[j].end > intervals->items[i].start) break;

//             // free register
//             active_registers[locations.items[active.items[j].vreg].register_index] = true;

//             // remove interval from active
//             int k = j;
//             while (k < active.count - 1) {
//                 active.items[k] = active.items[k + 1];
//                 k++;
//             }
//             active.count--;

//         }

//         // 4 is the amount of registers currently, will need to be changed for each platform
//         if (active.count == 4) {
//             // Spill interval i
//             assert(0);
//         } else {
//             // Allocate register
//             for (int k = 0; k < 4; k++) {
//                 if (active_registers[k]) {
//                     active_registers[k] = false;
//                     location.type = LOC_REGISTER;
//                     location.register_index = k;
//                     break;
//                 }
//             }

//             array_append(active, *interval);
//             int insert_slot = active.count - 1;
//             while (insert_slot > 0 && active.items[insert_slot - 1].end > interval->end) {
//                 active.items[insert_slot] = active.items[insert_slot - 1];
//                 insert_slot--;
//             }
//             active.items[insert_slot] = *interval;
//         }

//         locations.items[vreg] = location;
//     }
//     return locations;
// }


// // void print_live_intervals(IntervalArray intervals) {
// //     for (int i = 0; i < intervals.count; i++) {
// //         printf("r%d:\t[%d, %d)\n", intervals.items[i].vreg, intervals.items[i].start, intervals.items[i].end);
// //     }
// // }
