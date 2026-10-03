# 4. ANM
MieEngine was inspired by the series of games called "Touhou".

In Touhou, to draw all the complex animations that happen on the screen,
the games would use something called "ANM".

ANM is a bytecode format that's executed by a virtual machine (VM).

## Virtual machines
A virtual machine is a software component that
tries to emulate what real hardware does, with memory, registers, instructions, interrupts, etc.

When most people think of a VM, they think of a large program
that runs a complete OS, like VirtualBox, VMWare or QEMU.

But a VM is more than just that.

A lot of the things about how programmers understood computers in the past,
came from the theory of computation, which is a foundational branch of computer science
that studies automata, what can be computed by a machine,
and how efficiently it can be done.

One of the concepts of this theory is the Chomsky hierarchy.
It describes automata/languages based on their level of complexity and expressiveness.

A higher-level machine can describe any lower-level automata/language, in a strict inclusion model.

Type-3 automata would operate on so-called "Regular grammar".
You can think of it as an FSM, which takes in a symbol (from an array, or text)
and decides what to do with it. It operates on states,
which are finite variables that are modified by transitions.

Usual examples of type-3 machines would be regular expressions (Regex),
and also enemy AIs in some older video games.
The enemies would keep an enum of states, like Idle, Patrolling, Running, or Attacking.
They go from one state to another, based on rigid rules
(like `if see player -> go to Running state`,
`if player is at a certain distance to me -> go to Attack state`).

Type-3 automata are the simplest and most restrictive level.
If you were to make a paranthesis counter (like from the symbols `(((...)...))`),
that counts how many open-closed nested paranthesis pairs there are, using only an FSM,
then you would probably fail for a sufficiently large (infinite) array,
because transitions happen by rigidly setting variables to something.

It fails, because if the array's length is greater
than the fixed number of states that the FSM holds, so it would
no longer have any unique states, and it would repeat
those same states (it enters a cycle).
At that moment, the automata would forget which open parentheses it counted exactly,
giving you a wrong result, or an error.


The FSM doesn't do mathematical
calculations, like what we would do in programming,
so it's not really incrementing anything.
It only jumps from State A to State B.

The solution is to use a type-2, push-down automata,
which is what a VM is.

Type-2 automata stores an additional memory block, which might be a LIFO stack.
A stack is an array/string/data tape equipped with a stack pointer (sp).
Every time we encounter a symbol, we push and pop things to this stack.

Much like an FSM, these symbols are finite and can be uniquely differentiated
from one another.
In our previous example, every time the VM encounters an open parenthesis `(`,
it pushes something to the stack. If it encounters a closed parenthesis `)`,
it pops something from the stack.

Every parenthesis is closed correctly if the stack pointer of the VM is 0.
The VM introduces simple mathematical operations, by doing incrementing and decrementing
of a variable (the stack pointer, in this case).

And you know what mathematical operations are on real CPUs?
They involve taking a register and doing an operation with another register,
modifying the first register in the process.

Because of the LIFO nature of the Stack, the VM would only work with the stack top.

All of these registers can be considered relative memory addresses in a stack.
You first Push the values you want to add, and then,
when you encounter the Addition symbol, you Pop the first two values from the stack top,
and then you push the result back in.

```
PUSH 5
PUSH 3
ADD
PRINT     -> output: 8
```

Type-1 automata operate on so-called "Context-sensitive languages".
These languages have grammar rules where the understanding or validity
of a symbol depends on the characters around it.

For example, a programming language might use semantic validation (Semval)
to check if the code is syntactically-correct. Thus, in most PLs, you cannot use a variable
if it wasn't declared above (the context matters).

Type-0 automata are Turing machines. They can do any computational task, as long as they
have enough time and resources, aka. the memory and time are infinite.
And of course, in physics, there is no ideal Turing machine.
The memory tape is thus limited to a couple of gigabytes.

The Von-Neumann model is the primary architecture that's used by computers for decades,
and it's a type-0 automata. It has a CPU, RAM, which randomly stores the
program's data and instructions, and also the input-output systems.

Most programming languages, like C, C++, JS, etc are also type-0 automata.
This means that you can write any program that simulates another computer or an existing algorhithm.

We can say that the entire compilation process of a programming language
kinda acts as a heightening in the types of automatas in the Chomsky hierarchy.

In a lexer, the compiler reads the code character-by-character, ignores spaces and
groups these letters into tokens (`if`, `{`, `}`) (type-3 automata).

In a syntactic analyzer, you take these tokens and you form an AST of them.
This can be used to verify if the grammatical structure of the code
is correct (i.e. if you have correctly closed all the parentheses) (type-2 automata).

In Semval, you check the validity of the context surrounding the code.
For example, it might verify if a variable that you're trying to use in code
was declared previously, or that you're trying to adda number with a string,
which depends on the semantic context. (type-1 automata).

You lower the IR code into a language like C, Assembly or Bytecode,
which can be ran by a type-0 Turing automata or a VM.


## Applying virtual machines
In practice, most virtual machines can either be:
- Stack-based
- Register-based

Stack-based VMs are more in line with the theoretical definition
of a type-2 automata in the Chomsky hierarchy.

You push things to a stack and then you use instructions
to pop things from the stack and use them to achieve a certain result.

A register-based VM will use registers, which are few
memory boxes that are continuously changed as the VM is being ran.
This keeps the VM lightweight, because it reuses the very same memory regions,
as opposed to the stack's hundreds of elements.

Note that these instructions can be almost anything,
because they are symbols, as long as they can encode some useful information
for the VM.

### Instruction set architectures
Almost all VM formats will use an opcode, which is an unique
number that differentiates the instructions from one another,
thus making it easier for the VM to parse them individually.

In C and C++, enums can be used to represent these unique numbers.

Another element that instructions can have are the Arguments.
The arguments can act on the theoretical Stack of the VM,
or the instructions can do something more complex, like in the case of register-based VMs.

A more advanced VM can store some other information, like the current counter,
and some other immediate state.
This means that the instructions can also have some other information, too,
like time.



### Execution model
Now, how do we run our VM?

An automata, by its very theoretical nature,
will run with an infinite amount of resources and time,
or it can run up until we've reached the end of the list of symbols.

The list of symbols can be an array of characters or the aforementioned instructions
that are used by the VM.

In a game, it's desireable for the VM to not crash or freeze the entire game,
but it will only "halt" or wait for the next instructions to be executed,
so that the game loop can continue normally.

In order to make the game run smoothly (60 FPS, 16.7 ms/frame),
the VM only executes a part of the instructions each frame,
and then it temporarily stops (halts) or enters a state of waiting.

This allows the game loop to continue running, to render the graphics,
to poll the events from the keyboard/mouse, to update the rest of the logic, etc.


A pseudocode for the execution of a VM might be as follows:

```cpp
void VM_tick(VM* vm) {
    while (vm->currentInstruction != NULL) {
        if (vm->pendingInterrupt != 0) {
            // This is so that we can avoid the expensive switch cases.
            goto interrupt;
        }

        // Allow the game to execute smoothly.
        if (vm->currentInstruction->time > vm->timer) {
            return;
        }

        // Execute based on opcode.
        int opcode = vm->currentInstruction->opcode;
        switch (opcode) {
            // Accepted opcodes:
            case OP_ADD: {
                int lhs = vm->getArg(0);
                int rhs = vm->getArg(1);

                vm->putArg(0, lhs + rhs);
            } break;

            // etc.

            default: {
                puts("Unknown instruction!");
            } break;
        }

        // Get next instruction.
        if (currentInstruction != NULL) {
            // Load next instruction.
            vm->currentInstruction = vm->instructionBase + vm->currentInstruction->offset;
        }
    }
}
```

We use a switch statement to execute valid opcodes.
Otherwise, it prints the message "Unknown instruction!" to the screen.

The problem with a switch statement is that you have to check
through multiple conditions to see if it's the corresponding opcode,
but it's not really an issue if you have a relatively small amount of opcodes.

You can also use a dispatch table to execute other functions,
which is essential for FFI. The dispatch table stores the addresses
of the instructions that can be executed.

The performance bottleneck mostly comes from executing functions at disparate
memory addresses (like in the case of dispatch tables), rather than having conditional statements
that jump to close-together instruction addresses.

This is because the processor runs fast, but it has a slower RAM,
so CPU manufactuers started putting the Cache as a much faster memory embedded directly within the processor.
The CPU does not only load the current instruction from the RAM, but also
the following instructions (which is done at neightbouring memory addresses).

The compiler often puts the instructions from the switch statement close together in memory,
which is different from function pointers, where the difference in addresses between
two functions depends on the number of instructions that they have. Function pointers
have scattered memory addresses.

The CPU has to eventually jump from the `VM_tick()` function to the function specified by the pointer,
which is a expensive operation, so that's why FFI is done through a dedicated `OP_CALL` instruction.



Interrupts are probably the most complicated aspect of this code.


An interrupt is a mechanism where the computer temporarily stops its current activity, runs a
special list of instructions and then hands control back to the current instruction pointer,
towards the next instruction.

It's used for quickly responding to external priority events/signals in the VM.
You set the `vm->pendingInterrupt` to a value other than 0
before the `VM_tick()` function, and that function immediately responds
by locating the label that's marked by an interrupt opcode,
and then executing that label until it hits the interrupt opcode,
forcing the VM to go to the next instruction.

Another logical pattern that can be used are coroutines.
These are separate execution units that store the instruction pointer
and other memory state (registers, stacks or additional variables).
A stack in a coroutine has a stack base and a stack pointer.
The stack base is a pointer to a larger stack memory region,
that's determined in an equidistant way when the VM is initialized.

For scheduling, each coroutine has an active flag, and the VM simply iterates
over the active coroutines to execute the same code.


## ANM reference
An ANM virtual machine is a register-based VM.
It stores 4 integer variables, and 4 float variables,
that are used as registers.

ANM runs a certain set of instructions.
Each instruction contains the opcode,
an offset to the next instruction, 
a time taken for the instruction,
a variable bit mask that determines whether or not the argument at that bit
is a literal (1) or an address (0),
and then a list of arguments.

```cpp
typedef struct AnmVM_rawInstr {
    uint16_t opcode;
    uint16_t offset; // Offset to next instruction.
    short time;
    uint16_t varMask;
    int args[10];
} AnmVM_rawInstr;
// Size: 48 bytes.
```

These instructions include common ones like addition, subtraction, multiplication, division,
but also more graphical functions, like interpolations.

Linear interpolation is used in animation to move an object from one place to another (keyframes),
by smoothly changing its attributes (position, rotation, color, etc) over time.

By default, linear interpolation changes things over a straight line in the time domain,
which makes the movement seem robotic, but by using something called "Easing",
the movement becomes more natural, even though the functions end at the exact same time.
