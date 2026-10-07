#include "arena.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif


typedef struct ArenaHeapSystemInfo {
    u32 logicProcessorCount;
    u64 pageSize;
    u64 largePageSize;
    u64 allocGranularity;
} ArenaHeapSystemInfo;

static ArenaHeapSystemInfo arena_osInfo = { 0 };
static int arena_isCached = 0;

static void ArenaHeapGetSystemInfo(ArenaHeapSystemInfo **out)
{
    if (!arena_isCached)
    {
#if defined(_WIN32)
        SYSTEM_INFO sys_info = { 0 };
        GetSystemInfo(&sys_info);

        arena_osInfo.logicProcessorCount = (uint32_t) sys_info.dwNumberOfProcessors;
        arena_osInfo.pageSize = (uint64_t) KB(4);
        arena_osInfo.largePageSize = MB(2);
        arena_osInfo.allocGranularity = arena_osInfo.pageSize;
#else
        osInfo.logicProcessorCount = (uint32_t) get_nprocs();
        osInfo.pageSize = (uint64_t) getpagesize();
        osInfo.largePageSize = MB(2);
        osInfo.allocGranularity = os_info.page_size;
#endif
        arena_isCached = 1;
    }

    *out = &arena_osInfo;
}

static void* ArenaHeapReserve(size_t size)
{
#if defined(_WIN32)
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
#else
#endif
}

static int ArenaHeapRelease(void* ptr, size_t size)
{
#if defined(_WIN32)
    ARENA_UNUSED(size);
    return !VirtualFree(ptr, 0, MEM_RELEASE);
#else
#endif
}

static int ArenaHeapCommit(void* ptr, size_t size)
{
#if defined(_WIN32)
    void* ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return (ret == NULL);
#else
#endif
}

static int ArenaHeapDecommit(void* ptr, size_t size)
{
#if defined(_WIN32)
    return !VirtualFree(ptr, size, MEM_DECOMMIT);
#else
#endif
}

static void ArenaFatalError(const char* msg, int code)
{
#if defined(_WIN32)
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
    ExitProcess((u32) code);
#else
#endif
}




extern ARENA* ArenaInit(u64 reserveSize, u64 commitSize, u32 flags)
{
    ArenaHeapSystemInfo* sysinfo;
    ArenaHeapGetSystemInfo(&sysinfo);

    u64 pageSize = sysinfo->pageSize;

    reserveSize = ARENA_ALIGN_UP_POW2(reserveSize, pageSize);
    commitSize = ARENA_ALIGN_UP_POW2(commitSize, pageSize);

    ARENA* arena = (ARENA *) ArenaHeapReserve(reserveSize);
    ArenaHeapCommit(arena, commitSize);


    if (arena == NULL)
    {
        //fprintf(stderr, "%s: Failed to init arena! OOM?\n", __func__);
        // TODO: Do some print calls?
        ArenaFatalError("Failed to init arena! OOM?\n", -1);

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
        ArenaHeapRelease(current, current->reserveSize);

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

    u64 posAligned = ARENA_ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
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
                ArenaHeapSystemInfo* sysinfo;
                ArenaHeapGetSystemInfo(&sysinfo);

                u64 pageSize = sysinfo->pageSize;

                reserveSize = ARENA_ALIGN_UP_POW2(size + ARENA_HEADER_SIZE, pageSize);
            }

            ARENA* newArena = ArenaInit(reserveSize, commitSize, arena->flags);
            newArena->basePos = current->basePos + current->reserveSize;

            ARENA* prevCur = current;
            current = newArena;
            current->prev = prevCur;
            arena->current = current;

            posAligned = ARENA_ALIGN_UP_POW2(current->pos, ARENA_ALIGN);
            out = (u8 *) current + posAligned;
            newPos = posAligned + size;
        }
    }

    if (newPos > current->commitPos)
    {
        u64 newCommitPos = newPos;
        newCommitPos += current->commitSize - 1;
        newCommitPos -= newCommitPos % current->commitSize;
        newCommitPos = ARENA_MIN(newCommitPos, current->reserveSize);

        u64 commitSize = newCommitPos - current->commitPos;
        u8* commitPtr = (u8 *) current + current->commitPos;

        if (ArenaHeapCommit(commitPtr, commitSize)) {
            out = NULL;
        } else {
            current->commitPos = newCommitPos;
        }
    }

    if (out == NULL)
    {
        //fprintf(stderr, "%s: Failed to allocate memory on arena!\n", __func__);
        // TODO: Do some printing call?
        ArenaFatalError("Failed to allocate memory on arena!\n", -1);

        *((volatile unsigned long *) 0x00000000) = 0xffffffff;
    }

    current->pos = newPos;

    // If it's zero memory.
    if (!nonZero)
    {
        ARENA_MEMSET(out, 0, size);
        // TODO: NO. Add memset function.
    }


    return out;
}

extern void ArenaPop(ARENA* arena, size_t size)
{
    size = ARENA_MIN(size, ArenaGetPos(arena));

    // Iterate over arenas.
    ARENA* current = arena->current;
    while (current != NULL && size > current->pos)
    {
        ARENA* prev = current->prev;

        size -= current->pos;
        ArenaHeapRelease(current, current->reserveSize);

        current = prev;
    }

    arena->current = current;

    size = ARENA_MIN(current->pos - ARENA_HEADER_SIZE, size);
    current->pos -= size;

    if (arena->flags & ARENA_FLAG_DECOMMIT)
    {
        u64 requiredCommitPos = current->pos + current->commitSize - 1;
        requiredCommitPos -= requiredCommitPos % current->commitSize;

        if (requiredCommitPos < arena->commitPos)
        {
            u8* commitPtr = (u8 *) current + requiredCommitPos;

            if (ArenaHeapDecommit(commitPtr, arena->commitPos - requiredCommitPos))
            {
                //fprintf(stderr, "%s: Failed to decommit arena memory!\n", __func__);
                // TODO: Do some printing call?
                ArenaFatalError("Failed to decommit arena memory!\n", -1);

                *((volatile unsigned long *) 0x00000000) = 0xffffffff;
            }

            arena->commitPos = requiredCommitPos;
        }
    }
}

extern void ArenaPopTo(ARENA* arena, size_t pos)
{
    u64 currentPos = ArenaGetPos(arena);

    pos = ARENA_MIN(pos, currentPos);

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


static ARENA_THREAD_LOCAL ARENA* scratchArenas[2] = { 0 };

extern ARENA_TEMP ArenaScratchGet(ARENA **conflicts, u32 nConflicts)
{
    int scratchIndex = -1;

    for (int i = 0; i < 2; ++i)
    {
        b32 conflictFound = 0;

        for (int j = 0; j < nConflicts; ++j)
        {
            if (scratchArenas[i] == conflicts[j])
            {
                conflictFound = 1;
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
        return { 0 };
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
