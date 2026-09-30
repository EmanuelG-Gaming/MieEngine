#ifndef VM_H_
#define VM_H_ 1

#include "../base/base_defs.h"

#include "../mem/arena.h"

#define VM_CAPACITY 16

#define INTERRUPT_DEFAULT_INSTR_LIMIT 256

typedef enum Opcode {
    DUMMY,
    ENDL, // End parsing, return literal value
    ENDV, // End parsing, return stack top,
    PUSHL, // Push literal
    PUSHV, // Push value from stack
    POP,
    XCHG,
    ADD, SUB, MUL, DIV, IDIV, MOD, //IMOD,
    AND, OR, XOR, SHL, SHR,// SAR,
    NEG, NOT,
    EQU, NEQ, LES, LEQ, GTR, GEQ, //BEL, ABV,
    //AEQ,
    JMP, JZ, JNZ,
    CALL,
    RET, // Return
    RETL, // Return literal
    RETV, // Return and push stack top

    PRINTU, PRINTC, PRINT,

    MAX_OPCODE,
} Opcode;

typedef enum LiteralTypes {
    INSTR_LIT_INT,
    INSTR_LIT_CHAR,
    INSTR_LIT_POINTER,
    INSTR_LIT_STR,
} LiteralTypes;

typedef enum VM_flags {
    VM_STATUS_HALTED = (1 << 0),
} VM_flags;


typedef struct VM VM;
typedef struct InstrNode InstrNode;

typedef uintptr_t (*VM_FunctionSignature)(InstrNode*, VM*);


typedef struct InstrNode {
    Opcode op;
    uintptr_t arg;
} InstrNode;



typedef struct VM_Label {
    const char* name;

    uintptr_t beginAddr;
    uintptr_t endAddr;
    uintptr_t offset;
} VM_Label;

typedef struct VM_Segment {
    uintptr_t begin;
    uintptr_t offset;
} VM_Segment;

typedef struct Interrupt {
    InstrNode* instrAddrBegin;
    InstrNode* instrAddrEnd;
    int instrLimit;
} Interrupt;


typedef struct VM_Stack {
    uintptr_t* sp;
} VM_Stack;

typedef struct VM {
    struct VM* current;

    VM_Segment* textSegment;

    VM_FunctionSignature** dispatchTable;
    int nFunctions;

    InstrNode* pc;

    VM_Stack stack;

    VM_Label labels[16];
    int labelCount;

    Interrupt interrupts[4];
    u32 flags;
} VM;

typedef struct ScriptRuntime {
    ARENA* arena;

    VM_FunctionSignature* dispatchTable;
    int nDispatches;

    VM* vms[VM_CAPACITY];
    int vmCount;
} ScriptRuntime;

extern int ScriptInit(void);
extern void ScriptTerminate(void);

extern VM* ScriptCreateVM(void);


extern VM* VM_init(ARENA* arena);
extern void VM_terminate(VM* vm);

extern int VM_setInstructions(VM* vm, InstrNode* instrArr, int nInstr);
extern Interrupt* VM_setInterrupt(VM* vm, int interruptIdx, VM_Label label);

/*
   VM execution model.
*/
extern void VM_halt(VM* vm);
extern uintptr_t VM_tick(VM* vm);
// Interrupts.
extern uintptr_t VM_triggerInterrupt(VM* vm, int interruptIdx);


#endif /* VM_H_ */
