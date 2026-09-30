#include "../../ext/arena.cpp"

/*
#include "arena.h"
#include "../platform/platform.h"


//#include <string.h>
//#include <assert.h>


extern ARENA* ArenaInit(u64 reserveSize, u64 commitSize, u32 flags)
{
    HeapSystemInfo* sysinfo;
    HeapGetSystemInfo(&sysinfo);

    u64 pageSize = sysinfo->pageSize;

    reserveSize = ALIGN_UP_POW2(reserveSize, pageSize);
    commitSize = ALIGN_UP_POW2(commitSize, pageSize);

    ARENA* arena = (ARENA *) HeapReserve(reserveSize);
    HeapCommit(arena, commitSize);


    if (arena == NULL)
    {
        //fprintf(stderr, "%s: Failed to init arena! OOM?\n", __func__);
        // TODO: Do some print calls?
        Platform_fatalError("Failed to init arena! OOM?\n", -1);

        *((volatile unsigned long *) 0x00000000) = 0xffffffff;
    }

    arena->current = arena;
    arena->prev = NULL;

    arena->reserveSize = reserveSize;
    arena->commitSize = commitSize;

    arena->flags = flags;

    arena->basePos = 0;
    arena->pos = ARENA_HEADER_SIZE;
    arena->commitPos = commitSize;

    return arena;
}

extern void ArenaTerminate(ARENA* arena)
{
    ARENA* current = arena->current;

    // Iterate over chained arenas.
    while (current != NULL)
    {
        ARENA* prev = current->prev;
        HeapRelease(current, current->reserveSize);

        current = prev;
    }
}

extern u64 ArenaGetPos(ARENA* arena)
{
    return arena->current->basePos + arena->current->pos;
}

extern void* ArenaPush(ARENA* arena, size_t size, b32 nonZero)
{
    void* out = NULL;

    ARENA* current = arena->current;

    u64 posAligned = ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
    out = (u8 *) current + posAligned;
    u64 newPos = posAligned + size;

    // Make a new arena and chain it to the current arena.
    if (newPos > current->reserveSize)
    {
        out = NULL;

        if (arena->flags & ARENA_FLAG_GROWABLE)
        {
            u64 reserveSize = arena->reserveSize;
            u64 commitSize = arena->commitSize;

            if (size + ARENA_HEADER_SIZE > reserveSize)
            {
                HeapSystemInfo* sysinfo;
                HeapGetSystemInfo(&sysinfo);

                u64 pageSize = sysinfo->pageSize;

                reserveSize = ALIGN_UP_POW2(size + ARENA_HEADER_SIZE, pageSize);
            }

            ARENA* newArena = ArenaInit(reserveSize, commitSize, arena->flags);
            newArena->basePos = current->basePos + current->reserveSize;

            ARENA* prevCur = current;
            current = newArena;
            current->prev = prevCur;
            arena->current = current;

            posAligned = ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
            out = (u8 *) current + posAligned;
            newPos = posAligned + size;
        }
    }

    if (newPos > current->commitPos)
    {
        u64 newCommitPos = newPos;
        newCommitPos += current->commitSize - 1;
        newCommitPos -= newCommitPos % current->commitSize;
        newCommitPos = MIN(newCommitPos, current->reserveSize);

        u64 commitSize = newCommitPos - current->commitPos;
        u8* commitPtr = (u8 *) current + current->commitPos;

        if (HeapCommit(commitPtr, commitSize)) {
            out = NULL;
        } else {
            current->commitPos = newCommitPos;
        }
    }

    if (out == NULL)
    {
        //fprintf(stderr, "%s: Failed to allocate memory on arena!\n", __func__);
        // TODO: Do some printing call?
        Platform_fatalError("Failed to allocate memory on arena!\n", -1);
        
        *((volatile unsigned long *) 0x00000000) = 0xffffffff;
    }

    current->pos = newPos;

    // If it's zero memory.
    if (!nonZero)
    {
        //_MEMSET(out, 0, size);
        // TODO: NO. Add memset function.
    }


    return out;
}

extern void ArenaPop(ARENA* arena, size_t size)
{
    size = MIN(size, ArenaGetPos(arena));

    // Iterate over arenas.
    ARENA* current = arena->current;
    while (current != NULL && size > current->pos)
    {
        ARENA* prev = current->prev;

        size -= current->pos;
        HeapRelease(current, current->reserveSize);

        current = prev;
    }

    arena->current = current;

    size = MIN(current->pos - ARENA_HEADER_SIZE, size);
    current->pos -= size;

    if (arena->flags & ARENA_FLAG_DECOMMIT)
    {
        u64 requiredCommitPos = current->pos + current->commitSize - 1;
        requiredCommitPos -= requiredCommitPos % current->commitSize;

        if (requiredCommitPos < arena->commitPos)
        {
            u8* commitPtr = (u8 *) current + requiredCommitPos;

            if (HeapDecommit(commitPtr, arena->commitPos - requiredCommitPos))
            {
                //fprintf(stderr, "%s: Failed to decommit arena memory!\n", __func__);
                // TODO: Do some printing call?
                Platform_fatalError("Failed to decommit arena memory!\n", -1);

                *((volatile unsigned long *) 0x00000000) = 0xffffffff;
            }

            arena->commitPos = requiredCommitPos;
        }
    }
}

extern void ArenaPopTo(ARENA* arena, size_t pos)
{
    u64 currentPos = ArenaGetPos(arena);

    pos = MIN(pos, currentPos);

    ArenaPop(arena, currentPos - pos);
}


extern void ArenaFreeAll(ARENA* arena)
{
    // Erase positions only until we reach
    // the header.
    ArenaPopTo(arena, ARENA_HEADER_SIZE);
}


extern ARENA_TEMP ArenaTempBegin(ARENA* arena)
{
    return CLITERAL(ARENA_TEMP) {
        arena,
        ArenaGetPos(arena),
    };
}

extern void ArenaTempEnd(ARENA_TEMP temp)
{
    ArenaPopTo(temp.arena, 0);
}


static THREAD_LOCAL ARENA* scratchArenas[2] = { 0 };

extern ARENA_TEMP ArenaScratchGet(ARENA **conflicts, u32 nConflicts)
{
    int scratchIndex = -1;

    for (int i = 0; i < 2; ++i)
    {
        b32 conflictFound = FALSE;

        for (int j = 0; j < nConflicts; ++j)
        {
            if (scratchArenas[i] == conflicts[j])
            {
                conflictFound = TRUE;
                break;
            }
        }

        if (!conflictFound)
        {
            scratchIndex = i;
            break;
        }
    }

    if (scratchIndex == -1)
    {
        // Return null arena.
        return CLITERAL(ARENA_TEMP) { 0 };
    }

    if (scratchArenas[scratchIndex] == NULL)
    {
        scratchArenas[scratchIndex] = ArenaInit(
            ARENA_SCRATCH_RESERVE,
            ARENA_SCRATCH_COMMIT,
            ARENA_FLAG_GROWABLE
        );
    }

    return ArenaTempBegin(scratchArenas[scratchIndex]);
}

extern void ArenaScratchRelease(ARENA_TEMP scratch)
{
    ArenaTempEnd(scratch);
}
*/
