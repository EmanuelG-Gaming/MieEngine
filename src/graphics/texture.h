#ifndef TEXTURE_H_
#define TEXTURE_H_ 1

//#include "../mem/arena.h"

#define TEXTURE_POINT 1
#define TEXTURE_SRGB 2
#define TEXTURE_MIPMAP 4
#define TEXTURE_ALPHA 8
#define TEXTURE_HI_BIT 16

typedef struct GFX_texture {
    void* handle;
    void* resourceView;

    int w, h;
    int refs;
    int flags;
} GFX_texture;

extern int LoadPixels(int* w, int* h, unsigned char** data, int* channels, const char* assetPath, int flags);
extern GFX_texture* LoadTexture(void* alloc, const char* assetPath, int flags = 0);

extern void AllocateMutableTexture(int width, int height, void* args, int flags = 0);

// We load some texture here.
extern GFX_texture* LoadImmutableTextureFromPixels(void* alloc, int w, int h, unsigned char* pix, int flags = 0);

extern void ReleaseTexture(GFX_texture* texture);
extern void TextureTerminate(void* alloc, GFX_texture* texture);


#endif /* TEXTURE_H_ */
