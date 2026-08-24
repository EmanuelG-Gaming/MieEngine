#include "stack.h"
#include "mem.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static size_t CalcPaddingWithHeader(uintptr_t ptr, uintptr_t alignment, size_t headerSize)
{
    uintptr_t p, a, modulo, padding, needed_space;

    assert(IS_POW_2(alignment));

    p = ptr;
    a = alignment;
    modulo = p & (a - 1); // (p % a), as it assumes that
                          // alignment is a power of 2.
    padding = 0;
    needed_space = 0;

    if (modulo != 0)
    {
        padding = a - modulo;
    }

    needed_space = (uintptr_t) headerSize;

    if (padding < needed_space)
    {
        needed_space -= padding;
        if ((needed_space & (a - 1)) != 0) {
            padding += a * (1 + (needed_space / a));
        } else {
            padding += a * (needed_space / a);
        }
    }

    return (size_t) padding;
}

extern int StackInit(STACKALLOC *st, void *backingBuffer, size_t backingBufferLength)
{
    _MEMSET(st, 0, sizeof(*st));
    st->buf = (unsigned char *) backingBuffer;
    st->len = backingBufferLength;
    st->reservedSize = st->len;
    st->prev_offset = 0;
    st->curr_offset = 0;

    return 0;
}

extern int StackInit(STACKALLOC *st, size_t backingBufferLength)
{
    size_t reserve_size = backingBufferLength;
    size_t commit_size = backingBufferLength >> 1;

    HEAP_SYSTEMINFO* sysinfo;
    HeapGetSystemInfo(&sysinfo);

    reserve_size = ALIGN_UP_POW2(reserve_size, sysinfo->page_size);
    commit_size = ALIGN_UP_POW2(commit_size, sysinfo->page_size);

    void* backing = HeapReserve(reserve_size);
    HeapCommit(backing, commit_size);

    if (!backing)
    {
        fprintf(stderr, "%s: Failed to initialize stack memory!\n", __func__);
        *((volatile char *) 0x00) = 0xFF;
    }

    return StackInit(st, backing, reserve_size);
}

extern void StackTerminate(STACKALLOC* st)
{
    // TODO: This could cause a crash if the
    // stack's backing buffer is allocated using malloc.
    void* backing = st->buf;
    HeapRelease(backing, st->reservedSize);
}



extern void* StackAllocAlign(STACKALLOC* st, size_t size, size_t alignment)
{
    uintptr_t curr_addr, next_addr;
    size_t padding;

    STACKALLOCHEADER* header;

    assert(IS_POW_2(alignment));

    if (alignment > 128)
    {
        // Clamp to 128 bytes.
        alignment = 128;
    }

    curr_addr = (uintptr_t) st->buf + (uintptr_t) st->curr_offset;
    padding = CalcPaddingWithHeader(curr_addr, (uintptr_t) alignment, sizeof(STACKALLOCHEADER));
    if (st->curr_offset + padding + size > st->len)
    {
        return NULL;
    }
    st->prev_offset = st->curr_offset;
    st->curr_offset += padding;

    next_addr = curr_addr + (uintptr_t) padding;
    header = (STACKALLOCHEADER *) (next_addr - sizeof(STACKALLOCHEADER));
    header->padding = (uint8_t) padding;
    header->prev_offset = st->prev_offset;

    st->curr_offset += size;
    return _MEMSET((void *) next_addr, 0, size);
}


extern void* StackAlloc(STACKALLOC* st, size_t size)
{
    return StackAllocAlign(st, size, DEFAULT_ALIGNMENT);
}

extern int StackFree(STACKALLOC *st, void *ptr)
{
    if (ptr == NULL)
    {
        return -1;
    }

    uintptr_t start, end, currAddr;
    STACKALLOCHEADER* header;
    size_t prev_offset;

    start = (uintptr_t) st->buf;
    end = start + (uintptr_t) st->len;
    currAddr = (uintptr_t) ptr;

    if (!(start <= currAddr && currAddr < end))
    {
        fprintf(stderr, "%s: Out of bounds memory address!\n", __func__);
        return -1;
    }

    if (currAddr > start + (uintptr_t) st->curr_offset)
    {
        // Allow double frees.
        return 0;
    }

    // Go one block away.
    header = (STACKALLOCHEADER *) (currAddr - sizeof(STACKALLOCHEADER));
    prev_offset = (size_t) (currAddr - (uintptr_t) header->padding - start);
    if (prev_offset != header->prev_offset)
    {
        fprintf(stderr, "%s: Out of order stack free! LIFO principle.\n", __func__);
        return -1;
    }

    st->curr_offset = st->prev_offset;
    st->prev_offset = header->prev_offset;

    return 0;
}


extern int StackFreeAll(STACKALLOC *st)
{
    st->curr_offset = 0;
    return 0;
}


extern void* StackResizeAlign(STACKALLOC* st, void* ptr, size_t oldsize, size_t newsize, size_t alignment)
{
    if (ptr == NULL)
    {
        return StackAllocAlign(st, newsize, alignment);
    }
    else
    {
        uintptr_t start, end, currAddr;
        size_t minSize = oldsize < newsize ? oldsize : newsize;
        void* newPtr;

        start = (uintptr_t) st->buf;
        end = start + (uintptr_t) st->len;
        currAddr = (uintptr_t) ptr;
        if (!(start <= currAddr && currAddr < end))
        {
            fprintf(stderr, "%s: Failed to resize stack!\n", __func__);
            return NULL;
        }

        if (currAddr >= start + (uintptr_t) st->curr_offset)
        {
            // Treat as double free.
            return NULL;
        }

        if (oldsize == newsize)
        {
            return ptr;
        }

        newPtr = StackAllocAlign(st, newsize, alignment);
        _MEMMOVE(newPtr, ptr, minSize);
        return newPtr;
    }
}


extern void* StackResize(STACKALLOC *st, void *ptr, size_t oldsize, size_t newsize)
{
    return StackResizeAlign(st, ptr, oldsize, newsize, DEFAULT_ALIGNMENT);
}

