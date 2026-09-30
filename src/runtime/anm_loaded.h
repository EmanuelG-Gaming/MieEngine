#ifndef ANM_LOADED_H_
#define ANM_LOADED_H_ 1

#include "../base/base_defs.h"
#include "../graphics/texture.h"

typedef struct AnmHeader {
    u32 version;
    u16 numSprites;
    u16 numScripts;
    u16 zero1; // Actually zero.
    u16 w;
    u16 h;
    u16 formatIndex;
    u32 nameOffset;
    u16 x;
    u16 y;
    u32 memoryPriority;
    u32 textureOffset;
    u16 hasData;
    u16 lowresScale;
    u32 nextOffset;
    u32 zero2[6]; // Padding.
} AnmHeader;
// 64 bytes.
//STATIC_GETSIZE(AnmHeader);


/*
   I think you can use ANM VMs in a standalone way
   without using AnmManager, maybe.
*/

typedef struct TexLoaded {
    GFX_texture* texture;

    void* srcData;
    int srcDataSize;
    int bytesPerPixel;

    uint32_t flags;
} TexLoaded;

typedef struct TexLoadedSprite {

} TexLoadedSprite;


typedef struct AnmLoaded {
    TexLoaded* anmLoadeds;

    TexLoadedSprite* keyframeData;
    uint32_t* spriteData;

    AnmHeader* header;
    int numAnmLoadeds;

    int numScripts;
    int numSprites;

    int anmsLoading;
    int anmSlotIndex;
    char filePath[260];

    int texturesCreated;

    static void setupTextures(AnmLoaded* self);

} AnmLoaded;
#endif /* ANM_LOADED_H_ */
