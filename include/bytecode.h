#ifndef BYTECODE_H
#define BYTECODE_H

#include "ast.h"
#include "symbol_table.h"

typedef enum {
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_DIV,
    OPCODE_CMP_EQ,
    OPCODE_CMP_LT,
    OPCODE_CMP_GT,
    OPCODE_CMP_LEQ,
    OPCODE_CMP_GEQ,
    OPCODE_LOAD, // TODO: currently load and store operate on the stack.
               // will need to seperate stack loads from regular loads
    OPCODE_STORE,
    OPCODE_COPY,
    OPCODE_LOADI,
    OPCODE_RET,

    OPCODE_JMP,
    OPCODE_JMP_EQ,
    OPCODE_JMP_NEQ,
    OPCODE_CALL,
    OPCODE_PARAM,

    OPCODE_LABEL,      // TODO: might not be the best to conflict metadata on bytecode with operands. this is just for time being
    OPCODE_PROC_BEGIN, // dest is proc name, arg1 is max stack offset
    OPCODE_PROC_END,   // dest is proc name, arg1 is max stack offset
} Opcode;

typedef enum {
    OPERAND_INVALID, // dont know if this is best approach
    OPERAND_VREG,
    OPERAND_IMM,
    OPERAND_REG,
    OPERAND_STACK,
    OPERAND_LABEL, // LABEL, PROC_BEGIN, PROC_END
} OperandType;

typedef struct {
    OperandType type;
    union {
        int vreg;
        int reg;
        int imm;
        int stack_offset;
        char *label_name;
    };
} Operand;

typedef struct {
    Opcode opcode;
    Operand dest;
    Operand arg1;
    Operand arg2;
} Instr;

typedef struct {
    Instr *items;
    size_t count;
    size_t capacity;
} Bytecode;





typedef enum {
    LOC_REGISTER,
    LOC_STACK,
} LocationType;

typedef struct {
    LocationType type;
    union {
        int register_index;
        int stack_offset; // negative offset from rbp (so 8 means [bp - 8])
    };
} Location;

typedef struct {
    Location *items;
    size_t count;
    size_t capacity;
} LocationArray;

typedef struct {
    int vreg;
    int start; // inclusive
    int end;   // exclusive i.e. dies at end so can be merged with another interval
} Interval;

typedef struct {
    Interval *items;
    size_t count;
    size_t capacity;
} IntervalArray;

typedef struct {
    const char *name;
    int param_count;
    int return_count;
    int local_var_size;
    int vreg_count;

    Bytecode bytecode;
    LocationArray locations;
    IntervalArray intervals;
} Procedure;

typedef struct {
    Procedure *items;
    size_t count;
    size_t capacity;
} Program;

typedef struct {
    IntervalArray regs[4]; // TODO: change this to be dependent on the backend (but still static)
} PhysRegs; // Structure to record the info of hardware and ABI constraints on registers for allocator

extern const char *registers[];
extern const char *registers_8bit_low[];

Operand gen_add_instr(Bytecode *bytecode, Operand left, Operand right);
Operand gen_sub_instr(Bytecode *bytecode, Operand left, Operand right);
Operand gen_mul_instr(Bytecode *bytecode, Operand left, Operand right);
Operand gen_div_instr(Bytecode *bytecode, Operand left, Operand right);
/*void gen_cmp_eq_instr(Bytecode *bytecode, );
void gen_cmp_lt_instr(Bytecode *bytecode, );
void gen_cmp_gt_instr(Bytecode *bytecode, );
void gen_cmp_leq_instr(Bytecode *bytecode, );
void gen_cmp_geq_instr(Bytecode *bytecode, );*/
Operand gen_load_instr(Bytecode *bytecode, int stack_offset);
Operand gen_store_instr(Bytecode *bytecode, int stack_offset, int vreg);
// void gen_copy_instr(Bytecode *bytecode, );
Operand gen_loadi_instr(Bytecode *bytecode, int imm);
// void gen_ret_instr(Bytecode *bytecode, );

void gen_ast_bytecode(Bytecode *bytecode, AST *ast);
void gen_program(Program *program, AST *ast);
void analyze_program(Program *program, PhysRegs *pregs);
void pretty_print_bytecode(Bytecode bytecode);
void pretty_print_program(Program program);

IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode, int vreg_count);
void print_live_intervals(IntervalArray intervals);
LocationArray linear_scan_register_allocation(IntervalArray *intervals, int vreg_count, PhysRegs *pregs);

#endif
