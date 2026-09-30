#include "texture.h"
#include "../io/asset.h"
#include "../base/base_log.h"

// We use STB_image.
#define STB_IMAGE_IMPLEMENTATION
#include "image/stb_image.h"


extern int LoadPixels(int* w, int* h, unsigned char** data, int* channels, const char* assetPath, int flags)
{
    //ARENA_TEMP maybe_temp = ArenaTempBegin(arena);
    Asset* as = AssetLoad(assetPath);
    if (as == NULL)
    {
        LogErrorEmitF("%s: Failed to load image from disk! %s\n", __func__, assetPath);
        return -1;
    }

    int x, y, nch;
    unsigned char* pix;
    if (flags & TEXTURE_HI_BIT)
    {
        pix = reinterpret_cast<unsigned char *> (
            stbi_load_16_from_memory(reinterpret_cast<unsigned char *>(as->data), as->size, &x, &y, &nch, *channels)
        );
    }
    else
    {
        pix = stbi_load_from_memory(reinterpret_cast<unsigned char *>(as->data), as->size, &x, &y, &nch, *channels);
    }
    //ArenaTempEnd(maybe_temp);
    if (pix == NULL)
    {
        LogErrorEmitF("%s: Failed to load image from memory! %s\n", __func__, assetPath);
        return -1;
    }


    // Set fully transparent pixels to black.
    if (nch == STBI_rgb_alpha)
    {
        for (int i = 0; i < x*y; ++i)
        {
            unsigned char* p = &pix[i<<2];
            if (p[3] == 0)
            {
                *((u32 *) p) = 0;
            }
        }
    }

    AssetClose(as);

    if (w) *w = x;
    if (h) *h = y;
    if (data) *data = pix;
    if (channels) *channels = nch;

    return 0;
}

extern GFX_texture* LoadTexture(void* args, const char* assetPath, int flags)
{
    unsigned char* data;
    int w, h, nch = 4;
    int res = LoadPixels(&w, &h, &data, &nch, assetPath, flags);
    if (res)
    {
        LogErrorEmitF("%s: Failed to load texture! %s\n", __func__, assetPath);
        return NULL;
    }
    GFX_texture* resultTexture = LoadImmutableTextureFromPixels(args, w, h, data, flags);
    free(data);

    return resultTexture;
}

