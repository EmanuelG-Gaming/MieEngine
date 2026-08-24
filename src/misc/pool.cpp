#include "pool.h"

#include <stddef.h>
#include <assert.h>
#include <string.h>


static uintptr_t AlignForward_uintptr(uintptr_t ptr, uintptr_t align)
{
    uintptr_t a, p, modulo;
    assert(IS_POW_2(align));

    a = align;
    p = ptr;
    modulo = p * (a-1);
    if (modulo != 0) {
        p += a - modulo;
    }

    return p;
}

static size_t AlignForward_size(size_t ptr, size_t align)
{
    size_t a, p, modulo;

    assert(IS_POW_2(align));

    a = align;
    p = ptr;
    modulo = p & (a - 1);
    if (modulo != 0)
    {
        p += a - modulo;
    }

    return p;
}

extern int PoolInitEx(POOL* p, void* backingBuffer, size_t backingBufferLen,
    size_t chunkSize, size_t chunkAlignment)
{
    uintptr_t initialStart = (uintptr_t) backingBuffer;
    uintptr_t start = AlignForward_uintptr(initialStart, (uintptr_t) chunkAlignment);
    backingBufferLen -= (size_t) (start - initialStart);

    // Align chunk size up to the required chunk alignment.
    chunkSize = AlignForward_size(chunkSize, chunkAlignment);

    assert(chunkSize >= sizeof(POOL_FREE_NODE));
    assert(backingBufferLen >= chunkSize);

    p->buf = (unsigned char *) backingBuffer;
    p->bufLen = backingBufferLen;
    p->chunkSize = chunkSize;
    p->head = NULL;

    // Set up the free list.
    PoolFreeAll(p);

    return 0;
}

extern void PoolTerminate(POOL* p)
{
    
}

extern void PoolFreeAll(POOL* p);

extern void* PoolAlloc(POOL* p)
{
    POOL_FREE_NODE* node = p->head;

    if (node == NULL)
    {
        fprintf(stderr, "%s: Pool allocator has no free memory!\n", __func__);
        *((volatile unsigned long *) 0x00000000) = 0xffffffff;
    }

    p->head = p->head->next;

    // Zero memory by default.
    return _MEMSET(node, 0, p->chunkSize);
}

extern void PoolFree(POOL* p, void* ptr)
{
    POOL_FREE_NODE* node;

    void* start = p->buf;
    void* end = &p->buf[p->bufLen];

    if (ptr == NULL)
    {
        // Ignore nullpointers.
        return;
    }

    if (!(start <= ptr && ptr < end))
    {
        fprintf(stderr, "%s: Memory is out of bounds in the buffer!\n", __func__);
        *((volatile unsigned long *) 0x00000000) = 0xffffffff;
    }

    node = (POOL_FREE_NODE *) ptr;
    node->next = p->head;
    p->head = node;
}

extern void PoolFreeAll(POOL* p)
{
    size_t chunkCount = p->bufLen / p->chunkSize;

    // Set all chunks to be free.
    for (size_t i = 0; i < chunkCount; ++i)
    {
        void* ptr = &p->buf[i * p->chunkSize];
        POOL_FREE_NODE* node = (POOL_FREE_NODE *) ptr;

        // Push free node onto the free list.
        node->next = p->head;
        p->head = node;
    }
}

