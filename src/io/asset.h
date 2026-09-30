#ifndef ASSET_H_
#define ASSET_H_ 1

#include "../base/base_defs.h"
#include "../mem/arena.h"

typedef struct AssetArchiveHeader {
    char magic[4];
    i32 fileCount;
    i32 compressedOffset;
    i32 uncompressedOffset;
} AssetArchiveHeader;
#define ARCHIVE_HEADER_SIZE 16

typedef struct AssetFileData {
    i32 offset;
    i32 compressedSize;
    i32 uncompressedSize;
    char name[64];
} AssetFileData;
#define ARCHIVE_FILE_METADATA_SIZE (12 + 64)

typedef struct Asset {
    void* data;
    int size;
} Asset;




extern int AssetArchive(int slot, const char* filePath);
extern void AssetArchiveTerminate(void);


// args can be an Arena or smth.
extern Asset* AssetLoad(const char* filePath);
//extern Asset* AssetLoadFromArchive(const char* filePath, int slot);
extern void AssetClose(Asset* asset);



#endif /* ASSET_H_ */
