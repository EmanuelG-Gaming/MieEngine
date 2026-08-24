#ifndef MEM_DS_H_
#define MEM_DS_H_ 1

#include "base_defs.h"

/*
   Memory and data structures.
*/

/*
   Dynamic arrays.
*/

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256*16
#endif /* DA_INIT_CAP */

#define DECL_DA(name, T) \
    typedef struct name { \
        size_t count; \
        size_t capacity; \
        T* items; \
    } name; \

#define DA_Reserve(da, expected_capacity) do { \
    /* TODO: bro */ \
    if ((expected_capacity) > (da)->capacity) { \
        if ((da)->capacity == 0) { \
            (da)->capacity = DA_INIT_CAP; \
        } \
        while ((expected_capacity) > (da)->capacity) { \
            (da)->capacity *= 2; \
            fprintf(stderr, "cap:%d\n", (int)(da)->capacity); \
        } \
        (da)->items = DECLTYPE_CAST((da)->items) _REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
        _ASSERT((da)->items != NULL && "OOM?"); \
    } \
} while (0)

#define DA_Append(da, item) do { \
    DA_Reserve((da), (da)->count + 1); \
    (da)->items[(da)->count++] = (item); \
} while(0)

#define DA_Free(da) do { \
    if ((da)->items != NULL) _FREE((da)->items); \
} while (0)

#define DA_AppendMany(da, newItems, itemsCount) do { \
    DA_Reserve((da), (da)->count + (itemsCount)); \
    _MEMCPY((da)->items + (da)->count, (newItems), (itemsCount) * sizeof(*(da)->items)); \
    (da)->count += (itemsCount); \
} while(0)

// Stack-like operations.

#define DA_Pop(da) (da)->items[(_ASSERT((da)->count > 0), --(da)->count)]
#define DA_Push(da, item) DA_Append(da, item)
#define DA_First(da) (da)->items[(_ASSERT((da)->count > 0), 0)]
#define DA_Last(da) (da)->items[(_ASSERT((da)->count > 0), (da)->count - 1)]

// Simply swaps the elements.
#define DA_RemoveUnordered(da, idx) do { \
    size_t j = (idx); \
    _ASSERT(j < (da)->count); \
    (da)->items[j] = (da)->items[--(da)->count]; \
} while (0)


// Iterates using pointer.
#define DA_Foreach(T, it, da) for (T it = (da)->items; it < (da)->items + (da)->count; ++it)

#define DA_Count(da) (da).count

/*
   Silent dynamic array (SDA).
   It has a hidden header with the count and capacity.

   Memory structure:
   [header][items]
   And the user only has to use the item pointer.
*/

// Silent dynamic array metadata.
typedef struct SDA_HEADER {
    unsigned int count;
    unsigned int capacity;
} SDA_HEADER;

#define SDA_Push(da, item) do { \
    if (da == NULL) { \
        SDA_HEADER* hdr = (SDA_HEADER *) _MALLOC(sizeof(*da) * DA_INIT_CAP + sizeof(SDA_HEADER)); \
        hdr->count = 0; \
        hdr->capacity = DA_INIT_CAP; \
        da = (DECLTYPE_CAST(da) ((hdr) + 1)); \
    } \
    SDA_HEADER* h = ((SDA_HEADER *) (da) - 1); \
    if (h->count >= h->capacity) { \
        h->capacity = h->capacity + (h->capacity >> 1); \
        h = (SDA_HEADER *) _REALLOC(h, sizeof(*da) * h->capacity + sizeof(SDA_HEADER)); \
        da = (DECLTYPE_CAST(da) ((h) + 1)); \
    } \
    (da)[h->count++] = (item); \
} while (0)

#define SDA_Free(da) do { \
    _FREE(((SDA_HEADER *)(da) - 1)); \
} while (0)

#define SDA_Count(da) \
    (((SDA_HEADER *) da) - 1)->count



#endif /* MEM_DS_H_ */
