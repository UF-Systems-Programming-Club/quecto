#ifndef IR_H
#define IR_H

#include "ast.h"
#include "symbol_table.h"

typedef enum {
    PR_BP,
} PhysicalReg;


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
    OPCODE_RET,

    OPCODE_JMP,
    OPCODE_BEQ,
    OPCODE_BLT,
    OPCODE_BGT,
    OPCODE_BGE,
    OPCODE_BLE,
    OPCODE_BNE,
    OPCODE_CALL,
    OPCODE_PARAM,

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
        PhysicalReg reg;
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
    OPERAND_REG,
    OPERAND_IMM,
    OPERAND_MEM,
    OPERAND_GLOBAL
} OperandType;


typedef struct {
    OperandType type;
    union {
        int vreg;
        PhysicalReg reg;
        int slot;
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

typedef struct {
    int size;
    bool sign;
} VregInfo;


typedef struct {
    VregInfo *items;
    size_t count;
    size_t capacity;
} VregInfoTable;


typedef struct {
    int size;
    bool sign;
    bool address_taken;
} SlotInfo;


typedef struct {
    SlotInfo *items;
    size_t count;
    size_t capacity;
} SlotTable;

typedef struct {
    Operand dest;
} Phi;


typedef struct {

    Bytecode bytecode;
    Instr terminator;

    struct {
        int *items;
        size_t count, capacity;
    } predecessors;
} BasicBlock;


typedef struct {
    BasicBlock *items;
    int *rpo; // indexed by item id
    int *dominance; // indexed by item id
    size_t count, capacity;
    size_t entry_block;
} CFGraph;


typedef struct {
    VregInfoTable vregs;
    SlotTable slots; // we want slots to stay as vregs but will get demoted to mem under certain rules
    CFGraph cfg;
    int current_block;
    
    Arena *arena;
    SymbolTable *scope;
    HashTable slot_from_symbol;
    HashTable global_from_symbol;
} EmitContext;


typedef struct {
    CFGraph cfg;
    VregInfoTable vregs;
    SlotTable slots;
    const char *name;
} Procedure;


typedef struct {
    Procedure *items;
    size_t count;
    size_t capacity;
} Program;


Operand allocate_vreg_explicit(EmitContext *context, VregInfo info);
Operand allocate_vreg_type(EmitContext *context, QuectoType *type);
int allocate_block(EmitContext *context);
void start_block(EmitContext *context, int block_id);
void fallthrough_to(EmitContext *context, int block);
Operand slot_for(EmitContext *context, SymbolData *sym);
Operand global_for(EmitContext *context, const char *ident);

void add_predecessor(CFGraph *cfg, int block, int with);
void dominance(Arena *arena, CFGraph *cfg);

void emit_program(EmitContext *context, Program *into, AST *program);
void emit_procedure(EmitContext *context, Procedure *into, AST *procedure);
void emit_statement(EmitContext *context, AST *statement);
void emit_block(EmitContext *context, AST *block);
void emit_if(EmitContext *context, AST *_if);
void emit_if_chain(EmitContext *context, AST *_if, int end);
void emit_while(EmitContext *context, AST *_while);
void emit_return(EmitContext *context, AST *ret);
void emit_assign(EmitContext *context, AST *assign);
void emit_decl(EmitContext *context,  AST *decl);
Operand emit_expr(EmitContext *context, AST *expr);
Operand emit_call(EmitContext *context, AST *call, bool has_dest);
Operand emit_lhs(EmitContext *context, AST *lhs);

Operand emit_instr(EmitContext *context, Opcode opcode, size_t opcount, ...);
void emit_jmp(EmitContext *context, int target);
void emit_branch(EmitContext *context, int true_block, int false_block, AST *condition);
void emit_ret(EmitContext *context, Operand with);

void print_procedure(Procedure proc);
void print_program(Program program);

#endif
