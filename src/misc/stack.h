#ifndef STACK_H_
#define STACK_H_ 1

#include "../base/base_defs.h"


// Most CPU cache lines have 64 bytes.
#define DEFAULT_ALIGNMENT 16

typedef struct STACKALLOCHEADER {
    size_t prev_offset;
    size_t padding;
} STACKALLOCHEADER;

typedef struct STACKALLOC {
    unsigned char* buf;
    size_t len;
    size_t reservedSize;
    size_t prev_offset;
    size_t curr_offset;
} STACKALLOC;


extern int StackInit(STACKALLOC* st, void* backingBuffer, size_t backingBufferLength);
extern int StackInit(STACKALLOC* st, size_t backingBufferLength);

extern void StackTerminate(STACKALLOC* st);

extern void* StackAllocAlign(STACKALLOC* st, size_t size, size_t alignment);
extern void* StackAlloc(STACKALLOC* st, size_t size);

extern int StackFree(STACKALLOC* st, void* ptr);
extern int StackFreeAll(STACKALLOC* st);

extern void* StackResizeAlign(STACKALLOC* st, void* ptr, size_t oldsize, size_t newsize, size_t alignment);
extern void* StackResize(STACKALLOC *st, void *ptr, size_t oldsize, size_t newsize);


#endif /* STACK_H_ */
