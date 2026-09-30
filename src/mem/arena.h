#include "../../ext/arena.h"

/*
#ifndef ARENA_H_
#define ARENA_H_ 1

#include "../base/base_defs.h"
*/

/*
   Arena allocator.
   Currently implements the bump allocator, since
   an arena allocator would usually chain arenas in a linked list
   once their capacity reaches a maximum.
*/

/*
#define ARENA_HEADER_SIZE (sizeof(ARENA))
#define ARENA_ALIGN (2 * sizeof(void *))

#define ARENA_NUM_SCRATCH 2
#define ARENA_SCRATCH_RESERVE MB(64)
#define ARENA_SCRATCH_COMMIT KB(64)


typedef enum ARENA_FLAG {
    ARENA_FLAG_NONE = 0,
    ARENA_FLAG_GROWABLE = (1 << 0),
    ARENA_FLAG_DECOMMIT = (1 << 1),
} ARENA_FLAG;

typedef struct ARENA {
    // Chained linked list of arenas.
    struct ARENA* current;
    struct ARENA* prev;

    //unsigned char* buf;
    //size_t bufLen;
    //size_t prevOffset;
    //size_t currOffset;

    u64 reserveSize;
    u64 commitSize;

    u64 basePos;
    u64 pos;
    u64 commitPos;

    u32 flags;
} ARENA;

typedef struct ARENA_TEMP {
    ARENA* arena;
    u64 startPos;
} ARENA_TEMP;


//extern int ArenaInitEx(ARENA* a, void* backingBuffer, size_t backingBufferLen);
extern ARENA* ArenaInit(u64 reserveSize, u64 commitSize, u32 flags);
extern void ArenaTerminate(ARENA* a);

//extern void* ArenaAllocAlign(ARENA* a, size_t size, size_t align);
// TODO: Implement.
//extern void* ArenaAlloc(ARENA* a, size_t size);

extern void* ArenaPush(ARENA* arena, size_t size, b32 nonZero);
// Zeroes out the memory automatically.
#define ArenaPushStruct(arena, T) (T *) ArenaPush(arena, sizeof(T), 0)
#define ArenaPushStructNZ(arena, T) (T *) ArenaPush(arena, sizeof(T), 1)
#define ArenaPushArrayZero(arena, T, N) (T *) ArenaPush(arena, sizeof(T) * (N), 0)
// Uninitialized memory for speed.
#define ArenaPushArrayNZ(arena, T, N) (T *) ArenaPush(arena, sizeof(T) * (N), 1)






extern void* ArenaResizeAlign(ARENA* a, void* oldMemory, size_t oldSize, size_t newSize, size_t align);

extern void ArenaFreeAll(ARENA* arena);
#define ArenaClear(arena) ArenaFreeAll(arena)

extern void ArenaPop(ARENA* arena, size_t size);
extern void ArenaPopTo(ARENA* arena, size_t pos);

extern u64 ArenaGetPos(ARENA* arena);


extern ARENA_TEMP ArenaTempBegin(ARENA* arena);
extern void ArenaTempEnd(ARENA_TEMP temp);

extern ARENA_TEMP ArenaScratchGet(ARENA** conflicts, u32 nConflicts);
extern void ArenaScratchRelease(ARENA_TEMP scratch);




#endif */ /* ARENA_H_ */
