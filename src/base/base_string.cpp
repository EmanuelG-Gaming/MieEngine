#include "base_string.h"


extern int Str8Equal(STRING8 a, STRING8 b)
{
    if (a.len != b.len)
    {
        return FALSE;
    }

    for (int i = 0; i < a.len; ++i)
    {
        if (a.data[i] != b.data[i])
        {
            return FALSE;
        }
    }


    return TRUE;
}


extern STRING8 Str8Substr(STRING8 base, int a, int b)
{
    int m = a, M = b;
    if (b < a) {
        m = b;
        M = a;
    }

    return CLITERAL(STRING8) {
        (u64) (M - m),
        base.data + m,
    };
}
