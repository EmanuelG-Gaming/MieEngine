#include "asset.h"
#include <stdlib.h>

extern STRING8 AssetLoad(const char* filePath)
{
    FILE *file = fopen(filePath, "rb");
    if (!file)
    {
        fprintf(stderr, "%s: Couldn't read binary file at path: %s\n", __func__, filePath);
        return CLITERAL(STRING8) { 0 };
    }

    fseek(file, 0, SEEK_END);
    long buf_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (buf_size < 0)
    {
        fclose(file);
        return CLITERAL(STRING8) { 0 };
    }


    // We allocate a null-terminated text buffer with \0 at the end.
    u8* buf = (u8 *) _MALLOC(sizeof(u8) * (buf_size + 1));
    if (buf == NULL)
    {
        fprintf(stderr, "%s: OOM for buffer allocation?\n", __func__);
        return CLITERAL(STRING8) { 0 };
    }

    size_t new_len = fread(buf, sizeof(u8), buf_size, file);
    // Do some clamping.
    if (new_len > (size_t) buf_size)
    {
        new_len = (size_t) buf_size;
    }
    buf[new_len] = '\0';

    fclose(file);

    // With length-based strings:
    STRING8 ret = CLITERAL(STRING8)
    {
        new_len,
        buf,
    };

    return ret;

}
