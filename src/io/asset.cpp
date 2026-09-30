#include "asset.h"
#include "../base/base_log.h"

#include "../../ext/lzss.h"

#include <stdio.h>
#include <string.h>

#define ASSETS_ARCHIVE_COUNT 4

typedef struct LoadedArchive {
    FILE* fp;
    AssetArchiveHeader header;
    int isCompressed;
} LoadedArchive;

static LoadedArchive archives[ASSETS_ARCHIVE_COUNT];
//static int archiveCount = 0;



static int TryLoadFile(const char* path, void** dataBuf, int* size)
{
    FILE* fp = NULL;
    void* buf = NULL;
    int sz;

    fp = fopen(path, "rb");
    if (fp == NULL) goto err;

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = malloc(sz);
    if (buf == NULL) goto err;

    fread(buf, sz, 1, fp);
    fclose(fp);

    if (dataBuf != NULL) *dataBuf = buf;
    if (size != NULL) *size = sz;

    return TRUE;
err:
    if (buf != NULL) free(buf);
    if (fp != NULL) fclose(fp);
    return FALSE;
}



// TODO: Do this.

static int TryLoadAsset(const char* filePath, void** data, i32* compSize, int* decompSize, int slot)
{
    LoadedArchive* arch = &archives[slot];
    if (arch->fp == NULL)
    {
        fprintf(stderr, "%s: Failed to find archive at slot %d\n", __func__, slot);
        return FALSE;
    }

    if (filePath)
    {
        char buf[256];
        // TODO: Remove asset directory assets/ from search.
        //fprintf(stderr, "%s: comp offset: %d uncomp offset: %d\n", __func__, arch->header.compressedOffset, arch->header.uncompressedOffset);

        if (arch->isCompressed) {
            fseek(arch->fp, arch->header.compressedOffset, SEEK_SET);
        } else {
            fseek(arch->fp, arch->header.uncompressedOffset, SEEK_SET);
        }


        int foundInArchive = FALSE;
        AssetFileData fileData;
        for (int i = 0; i < arch->header.fileCount; ++i)
        {
            // First read the path.
            fread(&fileData, 1, sizeof(AssetFileData), arch->fp);

            // Check if it corresponds.
            if (!memcmp(fileData.name, filePath, strlen(filePath)))
            {
                foundInArchive = TRUE;
                break;
            }
        }

        if (!foundInArchive)
        {
            // Wasn't found in archive. Search for normal file instead.
            return FALSE;
        }

        if (data)
        {
            // Read continuous data.
            fseek(arch->fp, fileData.offset, SEEK_SET);

            if (arch->isCompressed)
            {
                char* tmp = new char[fileData.compressedSize];
                fread(tmp, 1, fileData.compressedSize, arch->fp);

                char* o = Lzss::decompress(tmp, fileData.compressedSize, NULL, fileData.uncompressedSize);

                // Generate dump of file.
                for (int i = 0; i < fileData.compressedSize; ++i)
                {
                    printf("%c ", o[i]);
                }
                //printf("%.*s\n", fileData.compressedSize, tmp);

                delete[] tmp;
                *data = o;
            }
            else
            {
                char* tmp = new char[fileData.uncompressedSize];
                fread(tmp, 1, fileData.uncompressedSize, arch->fp);
                //fprintf(stderr, "%s: Doing some things\n", __func__);

                *data = tmp;
            }
        }
        if (arch->isCompressed)
        {
            if (compSize) *compSize = fileData.compressedSize;
            if (decompSize) *decompSize = fileData.uncompressedSize;
        }
        else
        {
            if (compSize) *compSize = fileData.uncompressedSize;
            if (decompSize) *decompSize = fileData.uncompressedSize;
        }

        rewind(arch->fp);

        return TRUE;
    }

    return FALSE;
}

// TODO: Fix memory leaks.

extern Asset* AssetLoad(const char* filePath)
{
    // Check each archive for a file.
    void* data = NULL;
    int compSize = 0;
    int decompressedSize = 0;

    int i;
    b32 found = TryLoadFile(filePath, &data, &compSize);
    for (i = 0; i < ASSETS_ARCHIVE_COUNT && !found; ++i)
    {
        found = TryLoadAsset(filePath, &data, &compSize, &decompressedSize, i);
    }

    if (!found)
    {
        fprintf(stderr, "Couldn't find entry at: %s\n", filePath);
        return NULL;
    }

    // And then decompress the asset.
    //char* outData = ArenaPushArrayZero(arena, char, decompressedSize);
    //char* o = Lzss::decompress(reinterpret_cast<char *> (data), compSize, outData, decompressedSize);

    Asset* asset = new Asset;
    asset->data = data;
    asset->size = decompressedSize;

    return asset;
}

/*
extern Asset* AssetLoadFromArchive(ARENA* arena, const char* filePath, int slot)
{
    void* data = NULL;
    if (TryLoadAsset(filePath, &data, NULL, slot))
    {
        LogErrorEmitF("Failed to load asset from archive at slot %d from path %s", slot, filePath);
        return NULL;
    }

    return data;
}
*/




extern int AssetArchive(int slot, const char* filePath)
{
    LoadedArchive* arch = &archives[slot];
    if (arch->fp)
    {
        fclose(arch->fp);
        arch->fp = NULL;
    }

    if (filePath)
    {

        arch->fp = fopen(filePath, "rb");
        if (arch->fp == NULL)
        {
            LogErrorEmitF("Failed to open archive: %s", filePath);
            return -1;
        }

        // Read the magic signature.
        fread(&arch->header, 1, sizeof(arch->header), arch->fp);
        if (_MEMCMP(&arch->header.magic, "ARC0", 4))
        {
            LogErrorEmitF("Corrupted archive format! %s", filePath);
            fclose(arch->fp);
            return -1;
        }

        if (arch->header.compressedOffset == 0xFACEF00D)
        {
            arch->isCompressed = FALSE;
            fprintf(stderr, "[ARCHIVE] NOT COMPRESSED!\n");
        } else
        {
            arch->isCompressed = TRUE;
        }

    }

    return 0;

    /*
    // Check if matching paths exist.
    int pathFound = FALSE;
    for (int i = 0; i < archiveCount; ++i)
    {
        LoadedArchive* arch = &archives[i];
        if (!strcmp(filePath, arch->path))
        {
            LogErrorEmitF("Asset archive already loaded at path: %s", filePath);
            fclose(fp);
            return -1;
        }
    }

    AssetArchiveHeader header;
    strcpy(header.magic, magic);
    fread(&header.fileCount, 1, sizeof(i32), fp);
    fread(&header.compressedOffset, 1, sizeof(i32), fp);
    fread(&header.uncompressedOffset, 1, sizeof(i32), fp);
    // Go back to beginning.
    rewind(fp);

    LoadedArchive* arch = &archives[archiveCount];
    arch->fp = fp;
    arch->header = header;
    arch->path = filePath;

    archiveCount++;

    return 0;
    */
}

extern void AssetArchiveTerminate(void)
{
    /*
    for (int i = 0; i < archiveCount; ++i)
    {
        LoadedArchive* arch = &archives[i];
        if (arch->fp != NULL)
        {
            fclose(arch->fp);
            arch->fp = NULL;
        }
    }
    */
}


extern void AssetClose(Asset *asset)
{
    delete[] reinterpret_cast<char *> (asset->data);
}
