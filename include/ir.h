#ifndef IR_H
#define IR_H

#include "ast.h"
#include "common.h"
#include "symbol_table.h"



typedef enum {
    OPCODE_NONE,
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
    OPCODE_IMM,
    OPCODE_INDEX,
    OPCODE_ADDR,

    OPCODE_STORE,
    OPCODE_COPY,

    OPCODE_CALL,
    OPCODE_ARG, // defines args for call
    OPCODE_PARAM, // defines entry for vregs

    OPCODE_JMP,
    OPCODE_BEQ,
    OPCODE_BLT,
    OPCODE_BGT,
    OPCODE_BGE,
    OPCODE_BLE,
    OPCODE_BNE,
    OPCODE_RET,

    OPCODE_COUNT,
} Opcode;


typedef enum {
    ADDR_NONE = 0,
    ADDR_VREG,
    ADDR_REG,
    //ADDR_LABEL,
} AddrType; 


typedef struct {
    AddrType type;
    union {
        int vreg;
        int reg;
    };
} Addr;


typedef struct {
	Addr base;
	Addr index;
	int offset;
	int stride;
	int size;
} MemRef;


typedef enum {
    OPERAND_NONE = 0,
    OPERAND_VREG,
    OPERAND_SLOT,
    OPERAND_BLOCK, // for jumps and stuff
    OPERAND_REG,
    OPERAND_IMM,
    OPERAND_MEM,
    OPERAND_GLOBAL
} OperandType;


typedef struct {
    OperandType type;
    union {
        int vreg;
        int reg;
        int slot;
        int block;
        int imm;
        int glbl;
    		MemRef mem;
    };
} Operand;


typedef struct {
    Opcode opcode;
    Operand dest;
    Operand arg1;
    Operand arg2;
    int successor[2];
} Instr;


typedef struct {
    Instr *items;
    size_t count;
    size_t capacity;
} Bytecode;


typedef enum {
    PR_BASE_POINTER = 1,
    PR_STACK_POINTER = 2,
    PR_GENERAL_PURPOSE = 4,
    PR_CALLEE_SAVED = 8,
    PR_CALLER_SAVED = 16,
} PhysRegFlags;


typedef struct {
    int index;
    PhysRegFlags flags;
} Color;


typedef struct {
    int bstart, istart, bend, iend; // NOTE: this bend, iend is a simplication for linear scans in the future may be replaced with a set of end based on cfg
} Interval;


typedef struct {
    int size;
    bool sign;
    Interval interval;
    Color color;
    bool crosses_call;
    bool killed; // marked for death
} VregInfo;


typedef struct {
    VregInfo *items;
    size_t count;
    size_t capacity;
} VregInfoTable;


typedef struct {
    int size;
    bool sign;
    bool param;
    bool address_taken;
    bool killed; // marked for death
} SlotInfo;


typedef struct {
    SlotInfo *items;
    size_t count;
    size_t capacity;
} SlotTable;


typedef struct {
    int slot;
    Operand dest;
    Operand *args;
} Phi;

typedef struct {
    DYN_ARR(Phi) phis;
    DYN_ARR(int) predecessors;
    Bytecode bytecode;
    Instr terminator;
} BasicBlock;


typedef struct {
    BasicBlock *items;
    int *rpo, *rpo_list, *idom; // indexed by item id
    Set *df, *live_in, *live_out, *uses, *defines; // live_in -> defines are vreg liveness related
    size_t count, capacity, entry_block;
} CFGraph;


typedef struct {
    CFGraph cfg;
    Bytecode flattened;
    VregInfoTable vregs;
    int *labels;
    SlotTable slots;
    Set saved_colors;
    const char *name;
} Procedure;


typedef struct {
    SymbolTable *symbols;
    Procedure *items;
    size_t count;
    size_t capacity;
} Program;

extern const char *opcode_to_string[OPCODE_COUNT];
extern const Opcode opposite_opcode_table[OPCODE_COUNT];

// if arg is -1, matches anything
bool instr_match(Instr *instr, Opcode opcode, OperandType dest, OperandType arg1, OperandType arg2);
Operand allocate_vreg_explicit(Arena *arena, Procedure *procedure, VregInfo info);
bool vreg_defined(Instr instr, int vreg);
bool vreg_in_use(Instr instr, int vreg);
int vreg_if_use(Instr instr, int *vregs);
VregInfo vregi_from_sloti(SlotInfo slot);
bool instr_match(Instr *instr, Opcode opcode, OperandType dest, OperandType arg1, OperandType arg2);

void print_procedure(Procedure proc);
void print_program(Program program);
void print_cfg(CFGraph graph);
void print_block(CFGraph graph, bool walked[], int block);
void print_terminator(Instr instr);
void print_instruction(Instr instr);
void print_vregs(Procedure procedure);
void print_bytecode(Bytecode bytecode, size_t bsize, int blocks[bsize]);

#endif
