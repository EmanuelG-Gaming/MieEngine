#ifndef POOL_H_
#define POOL_H_ 1

#include "../base/base_defs.h"

/*
   Pool allocator.
   The pool allocator uses the data structure
   called the "free list", in order to keep track of the
   memory regions that could be used by this allocator.

   The memory blocks (elements) of this allocator all have the same size.
*/

//#ifndef DEFAULT_POOL_ALIGNMENT
//#define DEFAULT_POOL_ALIGNMENT 8
//#endif /* DEFAULT_POOL_ALIGNMENT */


#define POOL_ALIGN (2 * sizeof(void *))


typedef struct POOL_FREE_NODE {
    struct POOL_FREE_NODE* next;
} POOL_FREE_NODE;

typedef struct POOL {
    unsigned char* buf;
    size_t bufLen;
    size_t chunkSize;

    // Head of the free list.
    POOL_FREE_NODE* head;
} POOL;


// Pool + stack allocator maybe.

extern int PoolInitEx(POOL* p, void* backingBuffer, size_t backingBufferLen,
    size_t chunkSize, size_t chunkAlignment);
#define PoolInit(p, backing, len, chunkSize) \
    PoolInitEx(p, backing, len, chunkSize, POOL_ALIGN)

//extern int PoolInit(POOL* p, size_t len);

extern void PoolTerminate(POOL* p);

extern void PoolFreeAll(POOL* p);

extern void* PoolAlloc(POOL* p);
extern void PoolFree(POOL* p, void* ptr);

#endif /* POOL_H_ */
