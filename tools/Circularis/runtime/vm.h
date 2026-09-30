#ifndef CCL_VM_H_
#define CCL_VM_H_ 1

#include <stddef.h>
#include "../../../ext/arena.h"
#include <stdint.h>
#include "vm_instr.h"

#ifndef CCL_NON_NAMESPACE
namespace ccl {
#endif

#define VM_HEADER_SIZE 128

#define VM_CAPACITY 32

#define VM_COROUTINE_MAX 8

#define VM_DISPATCH_FUNCTION_COUNT 32
#define VM_DISPATCH_REGION_MAX 8


// 2048 bytes.
#define VM_STACK_SIZE 2048

#define VM_LAYER_CAPACITY 8


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

    EXEC,
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
//typedef struct InstrNode InstrNode;
typedef struct VM_rawInstr VM_rawInstr;

typedef uintptr_t (VM_functionSignature)(VM*);

/*
typedef struct InstrNode {
    Opcode op;
    uintptr_t arg;

    // A duration of 0 means that the instruction
    // would be executed instantly in a while loop.
    int16_t duration;
} InstrNode;
*/


typedef struct VM_segment {
    // Real address.
    uintptr_t begin;
    uintptr_t offset;
} VM_segment;

typedef struct VM_stack {
    uintptr_t* base;
    uintptr_t* sp;
} VM_stack;

/*
   Independent execution units on the same thread.
*/
typedef struct VM_coroutine {
    int active;
    int waitTime;

    VM_rawInstr* pc;
    uintptr_t* stack_base;
    uintptr_t* sp;
} VM_coroutine;

typedef struct VM {
    struct VM* current;

    VM_rawInstr* beginningOfScript;
    //VM_rawInstr* currentInstruction;

    VM_segment* textSegment;

    VM_functionSignature** dispatchTable;
    int nFunctions;

    VM_coroutine coroutines[VM_COROUTINE_MAX];
    int nCoroutines;

    VM_coroutine* currentRoutine;

    VM_stack stack;

    u32 flags;
} VM;

/*
typedef struct ScriptContext {
    // Dispatch table.
} ScriptContext;
*/

/*
   Some stack functions.
*/
extern void StackIncr(uintptr_t** sp);
extern void StackDecr(uintptr_t* sp);
extern void StackPush(uintptr_t** sp, uintptr_t value);
extern void StackPushi(uintptr_t** sp, int value);

extern uintptr_t StackPop(uintptr_t** sp);
extern uintptr_t* StackRef(uintptr_t* sp, int idx);
extern uintptr_t* StackTop(uintptr_t* sp);

extern uintptr_t PopVal(VM* vm);

extern uintptr_t InstrDispatchDirect(VM_rawInstr* pc, VM* vm);
extern uintptr_t InstrDispatch(VM_rawInstr* pc, VM* vm);

// Push bytecode into VM.
//extern int ANM_Push(ANM_VM* vm, u8* bytecode, int codeSize);

//extern VM* VM_Create(void);
//extern int VM_Tick(void);

extern VM* VM_Init(ARENA* arena);
extern void VM_Terminate(VM* vm);

extern int VM_SetInstructions(VM* vm, VM_rawInstr* instr, int nInstr);
extern int VM_SetFunctionsFFI(VM* vm, int slot, int nFunctions, VM_functionSignature** functions);

extern uintptr_t VM_UpdateCoroutine(VM* vm, VM_coroutine* coro);
extern void VM_Tick(VM* vm);

// Utility.
extern int PrintInstructions(VM_rawInstr* instructions, LiteralTypes* types, int nInstr);


static const char* OpcodeToStr(Opcode op)
{
    switch (op)
    {
        case DUMMY: return "dummy";
        case ENDL: return "endl";
        case ENDV: return "endv";
        case PUSHL: return "pushl";
        case PUSHV: return "pushv";
        case POP: return "pop";
        case XCHG: return "xchg";
        case ADD: return "add";
        case SUB: return "sub";
        case MUL: return "mul";
        case DIV: return "div";
        case IDIV: return "idiv";
        case MOD: return "mod";
        //case IMOD: return "imod";
        case AND: return "and";
        case OR: return "or";
        case LES: return "les";
        case LEQ: return "leq";
        case GTR: return "gtr";
        case GEQ: return "geq";
        case JMP: return "jmp";
        case JZ: return "jz";
        case JNZ: return "jnz";
        case CALL: return "call";
        case RET: return "ret";
        case RETL: return "retl";
        case RETV: return "retv";

        case EXEC: return "exec";
        case PRINTU: return "printu";
        case PRINTC: return "printc";
        case PRINT: return "print";

        case MAX_OPCODE: return "MAX OPCODE CONTROL VAR";
        default: return "UNKNOWN";
    }
}


#ifndef CCL_NON_NAMESPACE
} /* namespace ccl */
#endif
#endif /* CCL_VM_H_ */
