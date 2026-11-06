#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_ 1

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "types.h"

template <typename T>
class HashTable {
template <typename D>
struct TableBucket;
template <typename D>
struct TableList;

public:
    struct TableList<T>* begin;
    struct TableList<T>* end;

    TableList<T>* lists;
    int listCount;
    int capacity;

    int initialized;

    HashTable()
    {
        _MEMSET(this, 0, sizeof(HashTable));
    }

    ~HashTable()
    {
        if (lists != NULL)
        {
            for (TableList<T>* lst = begin; lst != NULL; lst = lst->next)
            {
                for (TableBucket<T>* ptr = lst->begin; ptr != lst->end; ptr = ptr->next)
                {
                    _FREE(ptr);
                }
            }

            _FREE(lists);
            lists = NULL;
        }
    }

    template <typename D>
    static void init(HashTable<D>* self, size_t capacity)
    {
        assert(!self->initialized);
        self->capacity = ((capacity + HASH_TABLE_ALIGN) & ~(HASH_TABLE_ALIGN+1));
        size_t bytecount = (sizeof(TableList<D>)) * self->capacity;
        self->lists = (TableList<T> *) _MALLOC(bytecount);
        assert(self->lists != NULL);
        _MEMSET(self->lists, 0, bytecount);

        self->initialized = TRUE;
    }


private:
    template <typename D>
    struct TableBucket {
        TableBucket* next;
        D elem;
    };

    template <typename D>
    struct TableList {
        TableBucket<D>* begin;
        TableBucket<D>* end;
        struct TableList* next;
        int occupied;
    };

    static constexpr size_t HASH_TABLE_ALIGN = 16;
};

#endif /* HASH_TABLE_H_ */
