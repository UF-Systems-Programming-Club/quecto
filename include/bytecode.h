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
    OPCODE_LOAD_INDEX,

    OPCODE_STORE,
    OPCODE_COPY,
    OPCODE_LOADI,
    OPCODE_RET,

    OPCODE_JMP,
    OPCODE_JMP_EQ,
    OPCODE_JMP_LT,
    OPCODE_JMP_GT,
    OPCODE_JMP_GE,
    OPCODE_JMP_LE,
    OPCODE_JMP_NEQ,
    OPCODE_CALL,
    OPCODE_PARAM,

    OPCODE_LABEL,      // TODO: might not be the best to conflict metadata on bytecode with operands. this is just for time being
    OPCODE_COUNT,
} Opcode;

typedef enum {
    OPERAND_INVALID = 0, // dont know if this is best approach
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

typedef enum {
    CC_INFERED,
    CC_SYSV, // for future use
} CallingConvention;

typedef struct {
    const char *name;
    int param_count;
    int return_count;
    CallingConvention convention;
    
    int local_var_size;
    int vreg_count;

    Bytecode bytecode;
    LocationArray locations; // keep locations and intervals apart of procedure for now, maybe this should be moved idk
    IntervalArray intervals;
} Procedure;

typedef struct {
    Procedure *items;
    size_t count;
    size_t capacity;
} Program;

typedef struct {
    SymbolTable *scope;
    const char *current_procedure_name;
} EmitContext;

typedef struct {
    IntervalArray regs[4]; // TODO: change this to be dependent on the backend (but still static)
} PhysRegs; // Structure to record the info of hardware and ABI constraints on registers for allocator

Operand gen_instr(Bytecode *bytecode, Opcode opcode, size_t opcount, ...); // operands should be passed as struct
Operand emit_expr_bytecode(EmitContext *context, Bytecode *bytecode, AST *expr);
void emit_if_bytecode(EmitContext *context, Bytecode *bytecode, AST *ifs);
void emit_while_bytecode(EmitContext *context, Bytecode *bytecode, AST *whiles);
void emit_decl_bytecode(EmitContext *context, Bytecode *bytecode, AST *decl);
void emit_assign_bytecode(EmitContext *context, Bytecode *bytecode, AST *assign);
void emit_block_bytecode(EmitContext *context, Bytecode *bytecode, AST *block);
void emit_return_bytecode(EmitContext *context, Bytecode *bytecode, AST *ret);
void emit_statement_bytecode(EmitContext *context, Bytecode *bytecode, AST *statement);
void emit_procedure_bytecode(EmitContext *context, Procedure *procedure, AST *ast);
void emit_program_bytecode(EmitContext *context, Program *program, AST *ast);

void analyze_program(Program *program, PhysRegs *pregs);
void pretty_print_bytecode(Bytecode bytecode);
void pretty_print_program(Program program);

IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode, int vreg_count);
void print_live_intervals(IntervalArray intervals);
LocationArray linear_scan_register_allocation(IntervalArray *intervals, int vreg_count, PhysRegs *pregs);

#endif
