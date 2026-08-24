#ifndef TEXTURE_H_
#define TEXTURE_H_ 1

#include "../base/base_defs.h"

#define TEXTURE_POINT 1
#define TEXTURE_SRGB 2
#define TEXTURE_MIPMAP 4

typedef struct TEXTURE {
    int w, h;
    int refs;
    int flags;
    void* handle;
    void* resourceView;
} TEXTURE;

extern TEXTURE* LoadImmutableTextureFromPixels(int w, int h, unsigned char* pix, int flags);
extern void TextureTerminate(TEXTURE* texture);

// RGBA white pixel square.
extern TEXTURE* Load2x2WhitePixelSquare(void);
extern TEXTURE* Load2x2Checkerboard(u32 mainDiag, u32 secDiag);

extern void GetAlignedUV(
    int tx, int ty, int blockW, int blockH, int atlasW, int atlasH, b32 flipY,
    float* u1, float* v1, float* u2, float* v2);


#endif /* TEXTURE_H_ */
