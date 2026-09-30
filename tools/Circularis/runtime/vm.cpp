#include "vm.h"
#include <stdio.h>

#ifndef CCL_NON_NAMESPACE
namespace ccl {
#endif

#define DISPATCH_CAPACITY 64
#define TEXT_SEGMENT_CAPACITY 2048


#ifdef __cplusplus
#define ALIGN_DECL(bytes) alignas(bytes)
#elif _MSC_VER
#define ALIGN_DECL(bytes) __declspec(align(bytes))
#else
#define ALIGN_DECL(bytes) __attribute((aligned(bytes)))
#endif

#define DECLTYPE_CAST(T) decltype(T)

#define SWAP_REF(a, b) do { \
    DECLTYPE_CAST((b)+0) tmp = (b); \
    *(a) = *(b); \
    *(b) = *(tmp); \
} while(0)


//static ANM_MANAGER anm = { 0 };



// NOTE: No bounds checking.

// Aligned uintptr_t.
extern inline void StackIncr(uintptr_t** sp)
{
    (*sp)++;
}
extern inline void StackDecr(uintptr_t** sp)
{
    (*sp)--;
}

// stack = struct { uintptr_t* sp; }
extern inline void StackPush(uintptr_t** sp, uintptr_t value)
{
    **(sp) = value;
    (*sp)++;
}
extern inline void StackPushi(uintptr_t** sp, int value)
{
    **(sp) = value;
    (*sp)++;
}

extern inline uintptr_t StackPop(uintptr_t** sp)
{
    (*sp)--;
    return **(sp);
}

extern inline uintptr_t* StackRef(uintptr_t* sp, int idx)
{
    return (uintptr_t *) (sp - idx);
}
extern inline uintptr_t* StackTop(uintptr_t* sp)
{
    return StackRef(sp, 0);
}

extern inline uintptr_t PopVal(VM* vm)
{
    return StackPop(&vm->currentRoutine->sp);
}



extern uintptr_t InstrDispatchDirect(VM* vm)
{
    VM_rawInstr* pc = vm->currentRoutine->pc;

    // NOTE: For debugging purposes.
    fprintf(stderr, "OPCODE: %s sp: %d\n", OpcodeToStr(static_cast<Opcode>(pc->opcode)),
            (int) (vm->currentRoutine->sp - vm->currentRoutine->stack_base));

    vm->currentRoutine->waitTime = pc->time;

    VM_functionSignature* fn = (vm->dispatchTable[pc->opcode]);
    return (*fn)(vm);
}
extern uintptr_t InstrDispatch(VM* vm)
{
    //vm->currentRoutine->pc += (vm->currentRoutine->pc->offsetToNextInstr);
    vm->currentRoutine->pc += 1;

    return InstrDispatchDirect(vm);
}


/*
   Putting 'inline' in a function for a V-table is risky,
   because the compiler can copy and paste the function.
*/

#define DISPATCH_IMPL

static DISPATCH_IMPL uintptr_t exec_dummy(VM* vm)
{
    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_endl(VM* vm)
{
    return vm->currentRoutine->pc->args[0];
}
static DISPATCH_IMPL uintptr_t exec_endv(VM* vm)
{
    return StackPop(&vm->currentRoutine->sp);
}
static DISPATCH_IMPL uintptr_t exec_pushl(VM* vm)
{
    // Push literal.
    StackPush(&vm->currentRoutine->sp, vm->currentRoutine->pc->args[0]);
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_pushv(VM* vm)
{
    // Push value from literal.
    StackPush(&vm->currentRoutine->sp, *StackRef(vm->currentRoutine->sp, vm->currentRoutine->pc->args[0]));
    return InstrDispatch(vm);
}

// Backtracking, essentially.
static DISPATCH_IMPL uintptr_t exec_pop(VM* vm)
{
    for (int i = 0; i < vm->currentRoutine->pc->args[0]; ++i)
    {
        StackPop(&vm->currentRoutine->sp);
    }
    return InstrDispatch(vm);
}


static DISPATCH_IMPL uintptr_t exec_xchg(VM* vm)
{
    uintptr_t* a = StackTop(vm->currentRoutine->sp);
    uintptr_t* b = StackRef(vm->currentRoutine->sp, vm->currentRoutine->pc->args[0]);

    SWAP_REF(a, b);
    return InstrDispatch(vm);
}

static DISPATCH_IMPL uintptr_t exec_add(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs + lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_sub(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs - lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_mul(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs * lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_imul(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs * lhs;
    return InstrDispatch(vm);
}

static DISPATCH_IMPL uintptr_t exec_div(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs / lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_idiv(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs / lhs;
    return InstrDispatch(vm);
}

static DISPATCH_IMPL uintptr_t exec_mod(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs % lhs;
    return InstrDispatch(vm);
}

/*
   Bitwise and logical operators.
*/

static DISPATCH_IMPL uintptr_t exec_and(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs & lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_or(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs | lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_xor(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs ^ lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_shl(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs << lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_shr(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs >> lhs;
    return InstrDispatch(vm);
}
/*
static inline uintptr_t exec_sar(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >> lhs;
    return InstrDispatch(pc, vm);
}
*/

static DISPATCH_IMPL inline uintptr_t exec_neg(VM* vm)
{
    uintptr_t operand = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = ~operand;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_not(VM* vm)
{
    uintptr_t operand = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = !operand;
    return InstrDispatch(vm);
}

static DISPATCH_IMPL uintptr_t exec_equ(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs == lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_neq(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs != lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_les(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs < lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_leq(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs <= lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_gtr(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs > lhs;
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_geq(VM* vm)
{
    uintptr_t rhs = StackPop(&vm->currentRoutine->sp);
    uintptr_t lhs = StackPop(&vm->currentRoutine->sp);
    *StackTop(vm->currentRoutine->sp) = rhs >= lhs;
    return InstrDispatch(vm);
}


static DISPATCH_IMPL uintptr_t exec_jmp(VM* vm)
{
    vm->currentRoutine->pc += vm->currentRoutine->pc->args[0];
    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_jz(VM* vm)
{
    size_t offset = StackPop(&vm->currentRoutine->sp) ? 1 : vm->currentRoutine->pc->args[0];
    vm->currentRoutine->pc += offset;

    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_jnz(VM* vm)
{
    size_t offset = StackPop(&vm->currentRoutine->sp) ? vm->currentRoutine->pc->args[0] : 1;
    vm->currentRoutine->pc += offset;

    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_call(VM* vm)
{
    // Push return address. (safety 1)
    StackPush(&vm->currentRoutine->sp, (uintptr_t) vm->currentRoutine->pc + 1);
    vm->currentRoutine->pc += vm->currentRoutine->pc->args[0];

    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_ret(VM* vm)
{
    vm->currentRoutine->pc = (VM_rawInstr *) StackPop(&vm->currentRoutine->sp);
    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_retl(VM* vm)
{
    // TODO: Unary op.
    return InstrDispatchDirect(vm);
}
static DISPATCH_IMPL uintptr_t exec_retv(VM* vm)
{
    return InstrDispatchDirect(vm);
}

/*
   FFI execution function.
*/
static DISPATCH_IMPL uintptr_t exec_exec(VM* vm)
{
    // Do some stack spilling (in reverse order).
    for (int i = 8; i >= 0; --i)
    {
        if ((vm->currentRoutine->pc->varMask >> i) & 1)
        {
            fprintf(stderr, "Called: i=%d\n", i);
            StackPush(&vm->currentRoutine->sp, vm->currentRoutine->pc->args[2 + i]);
            //fprintf(stderr, "EXEC SP:%d\n", (int) (vm->currentRoutine->sp - vm->currentRoutine->stack_base));
        }
    }

    fprintf(stderr, "bruh!!\n");

    VM_functionSignature* fn = vm->dispatchTable[(vm->currentRoutine->pc->args[0] * VM_DISPATCH_FUNCTION_COUNT) + vm->currentRoutine->pc->args[1]];
    fprintf(stderr, "%s: fn signature: %p", __func__, fn);
    return (fn)(vm);
}



/* Printing/debugging functions. */
static DISPATCH_IMPL uintptr_t exec_printu(VM* vm)
{
    printf("[VM PRINT]: %u\n", (unsigned int) *StackRef(vm->currentRoutine->sp, vm->currentRoutine->pc->args[0]));
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_printc(VM* vm)
{
    printf("[VM PRINT]: %c\n", (char) *StackRef(vm->currentRoutine->sp, vm->currentRoutine->pc->args[0]));
    return InstrDispatch(vm);
}
static DISPATCH_IMPL uintptr_t exec_print(VM* vm)
{
    // TODO: Print from text segment.
    //printf("[VM]: %s\n", (const char *) pc->args[0]);
    printf("[VM] TODO: NOT IMPLEMENTED TEXT SEGMENT!\n");
    return InstrDispatch(vm);
}

static DISPATCH_IMPL uintptr_t exec_none(VM* vm)
{
    return InstrDispatch(vm);
}



ALIGN_DECL(64) static VM_functionSignature* defaultDispatchTable[] = {
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

    exec_exec,
    exec_printu,
    exec_printc,
    exec_print,

    exec_none,
};


extern VM* VM_init(ARENA* arena)
{
    VM* vm = ArenaPushStruct(arena, VM);

    // Push stack.
    vm->stack.base = (uintptr_t *) ArenaPushArrayZero(arena, u8, VM_STACK_SIZE);
    vm->stack.sp = vm->stack.base;

    // Set up dispatch table.
    // We push the default dispatch table.
    size_t bytecount = sizeof(VM_functionSignature*) * VM_DISPATCH_FUNCTION_COUNT;
    vm->dispatchTable = (VM_functionSignature **) ArenaPushArrayZero(arena, u8, bytecount * VM_DISPATCH_REGION_MAX);
    memcpy(vm->dispatchTable, defaultDispatchTable, sizeof(defaultDispatchTable));

    fprintf(stderr, "%s: bytes: %zu\n", __func__, bytecount * VM_DISPATCH_REGION_MAX);

    vm->nFunctions = MAX_OPCODE;

    // Set up coroutines.
    vm->nCoroutines = 1;

    VM_coroutine* coroutine = &vm->coroutines[0];
    coroutine->active = TRUE;
    coroutine->waitTime = 0;
    vm->currentRoutine = coroutine;

    vm->flags = 0;

    return vm;
}

extern void VM_halt(VM* vm)
{
    vm->flags |= VM_STATUS_HALTED;
}

extern int VM_SetInstructions(VM* vm, VM_rawInstr* instr, int nInstr)
{
    // 16-byte alignment.
    size_t perCoroutineSize = ((VM_STACK_SIZE / vm->nCoroutines)) & ~0xF;

    // Set up ALL the coroutines.
    for (int i = 0; i < vm->nCoroutines; ++i)
    {
        VM_coroutine* routine = &vm->coroutines[i];
        routine->pc = instr;
        // Initial time pending.
        routine->waitTime = instr->time;

        // Sets up the stack pointer in an equidistant way.
        routine->stack_base = (uintptr_t *) (((u8 *) vm->stack.base) + (i * perCoroutineSize));
        routine->sp = routine->stack_base;

        //vm->pc = instr;
    }

    return 0;
}

extern int VM_SetFunctionsFFI(VM* vm, int slot, int nFunctions, VM_functionSignature** functions)
{
    if (slot == 0)
    {
        fprintf(stderr, "%s: Cannot push functions to reserved default opcodes! slot:%d\n", __func__, slot);
        return -1;
    }

    if (nFunctions >= VM_DISPATCH_FUNCTION_COUNT)
    {
        fprintf(stderr, "%s: Cannot push functions to dispatch table! The number of functions (%d) is too large.\n", __func__, nFunctions);
        return -1;
    }

    VM_functionSignature** fnSlot = (vm->dispatchTable) + (slot * VM_DISPATCH_FUNCTION_COUNT);

    fprintf(stderr, "%s: nfunctions: %d\n", __func__, nFunctions);
    // Copy memory.
    memcpy(fnSlot, functions, sizeof(VM_functionSignature*) * nFunctions);
    vm->nFunctions += nFunctions;


    return 0;
}



extern uintptr_t VM_UpdateCoroutine(VM* vm, VM_coroutine* coro)
{
    while (coro->waitTime < 0)
    {
        coro->waitTime--;
        return 0;
    }

    vm->currentRoutine = coro;

    return InstrDispatchDirect(vm);
}

extern void VM_Tick(VM* vm)
{
    if (vm->flags & VM_STATUS_HALTED)
    {
        return;
    }

    for (int i = 0; i < vm->nCoroutines; ++i)
    {
        VM_coroutine* coro = &vm->coroutines[i];

        if (coro->active)
        {
            //vm->currentRoutine = coro;

            uintptr_t res = VM_UpdateCoroutine(vm, coro);

            if (res != 0)
            {
                // Causes the whole VM to halt.
                ccl::VM_halt(vm);
                break;
            }
        }
    }
}



#ifndef CCL_NON_NAMESPACE
} /* namespace ccl */
#endif
