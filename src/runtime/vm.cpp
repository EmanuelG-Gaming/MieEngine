#include "vm.h"
#include "../base/base_log.h"

#include <string.h>

static ScriptRuntime scriptRuntime = { 0 };


#define DISPATCH_CAPACITY 64
#define TEXT_SEGMENT_CAPACITY 2048

#define VM_STACK_SIZE 512

#define SWAP_REF(a, b) do { \
    DECLTYPE_CAST((b)+0) tmp = (b); \
    *(a) = *(b); \
    *(b) = *(tmp); \
} while(0)


/*
   Stack operations.
*/

// NOTE: No bounds checking.

static inline void StackIncr(VM_Stack* stack)
{
    ++stack->sp;
}
static inline void StackDecr(VM_Stack* stack)
{
    --stack->sp;
}

// stack = struct { uintptr_t* sp; }
static inline void StackPush(VM_Stack* stack, uintptr_t value)
{
    *(stack->sp) = value;
    stack->sp++;
}

static inline uintptr_t StackPop(VM_Stack* stack)
{
    stack->sp--;
    return *(stack->sp);
}

static inline uintptr_t* StackRef(VM_Stack* stack, int idx)
{
    return (uintptr_t *) (stack->sp - idx);
}
static inline uintptr_t* StackTop(VM_Stack* stack)
{
    return StackRef(stack, 0);
}


/*
   Dispatching.
*/

static uintptr_t InstrDispatchDirect(InstrNode* pc, VM* vm)
{
    //fprintf(stderr, "%s: Global dispatchTable:%p\n", __func__, anm.dispatchTable);
    //fprintf(stderr, "%s: op=%s (%d) dispatchTable ptr:%p\n", __func__, OpcodeToStr(pc->op), pc->op, anm.dispatchTable[pc->op]);
    return scriptRuntime.dispatchTable[pc->op](pc, vm);
}
static uintptr_t InstrDispatch(InstrNode* pc, VM* vm)
{
    return InstrDispatchDirect(++pc, vm);
}


/*
   Some default dispatch functions.
*/

#define DISPATCH_IMPL

static DISPATCH_IMPL uintptr_t exec_dummy(InstrNode* pc, VM* vm)
{
    return InstrDispatchDirect(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_endl(InstrNode* pc, VM* vm)
{
    return pc->arg;
}
static DISPATCH_IMPL uintptr_t exec_endv(InstrNode* pc, VM* vm)
{
    return StackPop(&vm->stack);
}
static DISPATCH_IMPL uintptr_t exec_pushl(InstrNode* pc, VM* vm)
{
    // Push literal.
    StackPush(&vm->stack, pc->arg);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_pushv(InstrNode* pc, VM* vm)
{
    // Push value from literal.
    StackPush(&vm->stack, *StackRef(&vm->stack, pc->arg));
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_pop(InstrNode* pc, VM* vm)
{
    for (int i = 0; i < pc->arg; ++i)
    {
        StackPop(&vm->stack);
    }
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_xchg(InstrNode* pc, VM* vm)
{
    uintptr_t* a = StackTop(&vm->stack);
    uintptr_t* b = StackRef(&vm->stack, pc->arg);

    SWAP_REF(a, b);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_add(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs + lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_sub(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs - lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_mul(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs * lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_imul(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs * lhs;
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_div(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs / lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_idiv(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs / lhs;
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_mod(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs % lhs;
    return InstrDispatch(pc, vm);
}

/*
   Bitwise and logical operators.
*/

static DISPATCH_IMPL uintptr_t exec_and(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs & lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_or(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs | lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_xor(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs ^ lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_shl(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs << lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_shr(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >> lhs;
    return InstrDispatch(pc, vm);
}
/*
static inline uintptr_t exec_sar(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >> lhs;
    return InstrDispatch(pc, vm);
}
*/

static DISPATCH_IMPL inline uintptr_t exec_neg(InstrNode* pc, VM* vm)
{
    uintptr_t operand = StackPop(&vm->stack);
    *StackTop(&vm->stack) = ~operand;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_not(InstrNode* pc, VM* vm)
{
    uintptr_t operand = StackPop(&vm->stack);
    *StackTop(&vm->stack) = !operand;
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_equ(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs == lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_neq(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs != lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_les(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs < lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_leq(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs <= lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_gtr(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs > lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_geq(InstrNode* pc, VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >= lhs;
    return InstrDispatch(pc, vm);
}


static DISPATCH_IMPL uintptr_t exec_jmp(InstrNode* pc, VM* vm)
{
    return InstrDispatchDirect(pc + pc->arg, vm);
}
static DISPATCH_IMPL uintptr_t exec_jz(InstrNode* pc, VM* vm)
{
    size_t offset = StackPop(&vm->stack) ? 1 : pc->arg;
    return InstrDispatchDirect(pc + offset, vm);
}
static DISPATCH_IMPL uintptr_t exec_jnz(InstrNode* pc, VM* vm)
{
    size_t offset = StackPop(&vm->stack) ? pc->arg : 1;
    return InstrDispatchDirect(pc + offset, vm);
}
static DISPATCH_IMPL uintptr_t exec_call(InstrNode* pc, VM* vm)
{
    // Push return address.
    StackPush(&vm->stack, (uintptr_t) pc + 1);
    return InstrDispatchDirect(pc + pc->arg, vm);
}
static DISPATCH_IMPL uintptr_t exec_ret(InstrNode* pc, VM* vm)
{
    pc = (InstrNode *) StackPop(&vm->stack);
    return InstrDispatchDirect(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_retl(InstrNode* pc, VM* vm)
{
    // TODO: Unary op.
    return InstrDispatchDirect(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_retv(InstrNode* pc, VM* vm)
{
    return InstrDispatchDirect(pc, vm);
}

/* Printing/debugging functions. */
static DISPATCH_IMPL uintptr_t exec_printu(InstrNode* pc, VM* vm)
{
    LogInfoEmitF("[VM]: %u\n", (unsigned int) pc->arg);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_printc(InstrNode* pc, VM* vm)
{
    LogInfoEmitF("[VM]: %c\n", (char) pc->arg);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_print(InstrNode* pc, VM* vm)
{
    LogInfoEmitF("[VM]: %s\n", (const char *) pc->arg);
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_none(InstrNode* pc, VM* vm)
{
    return InstrDispatch(pc, vm);
}



ALIGN_DECL(64) static const VM_FunctionSignature defaultDispatchTable[] = {
    exec_dummy,
    exec_endl,
    exec_endv,
    exec_pushl,
    exec_pushv,
exec_pop,
    exec_xchg,
    exec_add, exec_sub, exec_mul, exec_div, exec_idiv, exec_mod,
    exec_and, exec_or, exec_xor, exec_shl, exec_shr,
    exec_neg, exec_not,
    exec_equ, exec_neq, exec_les, exec_leq, exec_gtr, exec_geq,

    exec_jmp, exec_jz, exec_jnz,
    exec_call,
    exec_ret, // Return.
    exec_retl, // Return literal.
    exec_retv, // Return and push stack pos.

    exec_printu,
    exec_printc,
    exec_print,

    exec_none,
};


static VM_Label defaultLabel = CLITERAL(VM_Label) { "main", 0, 0, 0 };

extern VM* VM_init(ARENA* arena)
{
    VM* vm = ArenaPushStruct(arena, VM);
    vm->current = vm;
    vm->labelCount = 0;

    vm->flags = 0;

    return vm;
}

extern void VM_terminate(VM *vm)
{
    // We don't do anything atm.
    UNUSED(vm);
}

extern int VM_setInstructions(VM* vm, InstrNode* instr, int nInstr)
{
    vm->pc = instr;
    return 0;
}
extern Interrupt* VM_setInterrupt(VM *vm, int interruptIdx, const char* labelName)
{
    // Find label.
    int labelIdx = -1;
    for (int i = 0; i < vm->labelCount; ++i)
    {
        if (!strcmp(labelName, vm->labels[i].name))
        {
            labelIdx = i;
            break;
        }
    }

    VM_Label label = { 0 };
    if (labelIdx == -1)
    {
        // We use the default label.
        label = defaultLabel;
    }
    else
    {
        label = vm->labels[labelIdx];
    }


    Interrupt* interrupt = &vm->interrupts[interruptIdx];

    interrupt->instrLimit = INTERRUPT_DEFAULT_INSTR_LIMIT;
    interrupt->instrAddrBegin = vm->pc + (label.offset + label.beginAddr);
    // Usually a "ret" instruction.
    interrupt->instrAddrEnd = vm->pc + (label.offset + label.endAddr);

    return interrupt;
}



/*
   VM execution model.
*/

extern void VM_halt(VM* vm)
{
    vm->flags |= VM_STATUS_HALTED;
}

extern uintptr_t VM_tick(VM* vm)
{

    if (vm->flags & VM_STATUS_HALTED)
    {
        return 0;
    }

    //Opcode op;
    /*
    sie_t i = 0;
    do
    {
    } while (op != ENDL && op != ENDV);
    */
    uintptr_t ret = InstrDispatchDirect(vm->pc, vm);
    //vm->pc += 1;

    //fprintf(stderr, "%s: ret: %d\n", __func__, (int) ret);

    return ret;
}
extern uintptr_t VM_triggerInterrupt(VM *vm, int interruptIdx)
{
    if (vm->flags & VM_STATUS_HALTED)
    {
        return 0;
    }

    Interrupt* interrupt = &vm->interrupts[interruptIdx];
    int i = 0;
    uintptr_t ret = -1;

    InstrNode* prev_pc = vm->pc;
    vm->pc = interrupt->instrAddrBegin;

    do {
        ret = InstrDispatchDirect(vm->pc, vm);
        i++;
    } while (i < interrupt->instrLimit && vm->pc != interrupt->instrAddrEnd);

    vm->pc = prev_pc;

    return ret;
}




/*
   Some preset data.
*/

static InstrNode defaultInstr[] = {
    {
        PRINTC,
        (uintptr_t) 'c',
    },
    {
        ENDL,
        67, // Haha funny number
    },
};



/*
   And then the script runtime.
*/

extern int ScriptInit(void)
{
    _MEMSET(&scriptRuntime, 0, sizeof(scriptRuntime));


    scriptRuntime.arena = ArenaInit(MB(4), KB(16), ARENA_FLAG_GROWABLE);
    scriptRuntime.vmCount = 0;

    size_t bytecount = sizeof(defaultDispatchTable);
    // We push the default dispatch table.
    scriptRuntime.dispatchTable = (VM_FunctionSignature *) ArenaPushArrayZero(scriptRuntime.arena, u8, bytecount);
    _MEMCPY(scriptRuntime.dispatchTable, defaultDispatchTable, bytecount);

    scriptRuntime.nDispatches = MAX_OPCODE;

    return 0;
}

extern void ScriptTerminate(void)
{
    ArenaTerminate(scriptRuntime.arena);
}

extern VM* ScriptCreateVM(void)
{
    VM* vm = VM_init(scriptRuntime.arena);

    // TODO: Maybe do this when we declare a text segment in the custom Assembly format.
    vm->textSegment = (VM_Segment *) ArenaPushArrayZero(scriptRuntime.arena, u8, TEXT_SEGMENT_CAPACITY);
    vm->textSegment->begin = (uintptr_t) vm->textSegment;
    vm->textSegment->offset = 0;

    // Fill in some data (we push the VM and the stack buffer in the same arena).
    uintptr_t* stackBuffer = (uintptr_t *) ArenaPushArrayZero(scriptRuntime.arena, u8, VM_STACK_SIZE);
    vm->stack.sp = stackBuffer;

    // DEFAULT VALUES!
    // We set some instructions (for default);
    VM_setInstructions(vm, defaultInstr, STATIC_ARR_LEN(defaultInstr));

    return vm;
}
