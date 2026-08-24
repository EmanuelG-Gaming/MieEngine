#include "texture.h"
#include "../base/base_defs.h"

extern TEXTURE* Load2x2WhitePixelSquare(void)
{
    static const u32 texData[4] = {
        0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff,
    };

    TEXTURE* result = LoadImmutableTextureFromPixels(2, 2,
        (unsigned char *) texData, 0);
    return result;
}

extern TEXTURE* Load2x2Checkerboard(u32 mainDiag, u32 secDiag)
{
    static const u32 texData[4] = {
        mainDiag, secDiag,
        secDiag, mainDiag,
    };

    TEXTURE* result = LoadImmutableTextureFromPixels(2, 2,
        (unsigned char *) texData, 0);
    return result;
}


extern void GetAlignedUV(
    int tx, int ty, int blockW, int blockH, int atlasW, int atlasH, b32 flipY,
    float* u1, float* v1, float* u2, float* v2)
{
    float w = (float) atlasW;
    float h = (float) atlasH;

    *u1 = (tx * blockW) / w;
    *v1 = (ty * blockH) / h;
    *u2 = ((tx + 1) * blockW) / w;
    *v2 = ((ty + 1) * blockH) / h;

    if (flipY)
    {
        *v1 = 1 - *v1;
        *v2 = 1 - *v2;
    }
}

