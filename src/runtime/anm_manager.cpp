#include "anm_manager.h"
//#include "../platform/platform.h"
#include "../io/asset.h"
#include "anm_vm.h"
#include "../base/base_string.h"
#include "../platform/platform.h"

#include <stddef.h>

#include <stdio.h>

AnmManager* AnmManager::init(AnmManager *self)
{
    self->primaryVm = AnmVM();
    for (int i = 0; i < 4096; ++i)
    {
        self->fastVms[i].init(&self->fastVms[i]);
    }

    return self;
}

AnmManager::~AnmManager()
{
    AnmVM_listNode* primaryHead = primaryGlobalHead;
    while (primaryHead)
    {
        AnmVM_listNode* primaryNext = primaryHead->next;
        AnmManager::removeVm(this, primaryHead->entry);
        primaryHead = primaryNext;
    }

    AnmVM_listNode* secondaryHead = secondaryGlobalHead;
    while (secondaryHead)
    {
        AnmVM_listNode* secondaryNext = secondaryHead->next;
        AnmManager::removeVm(this, secondaryHead->entry);
        secondaryHead = secondaryNext;
    }
}

void AnmManager::removeVm(AnmManager *self, AnmVM *vm)
{
    AnmVM_listNode* node = &vm->globalListNode;
    if (self->primaryGlobalTail == node)
    {
        self->primaryGlobalTail = node->prev;
    }
    if (self->primaryGlobalHead == node)
    {
        self->primaryGlobalHead = node->next;
    }

    if (self->secondaryGlobalTail == node)
    {
        self->secondaryGlobalTail = node->prev;
    }
    if (self->secondaryGlobalHead == node)
    {
        self->secondaryGlobalHead = node->next;
    }

    if (node->next)
    {
        node->next->prev = node->prev;
    }
    if (node->prev)
    {
        node->prev->next = node->next;
    }

    node->next = NULL;
    node->prev = NULL;

    AnmVM_listNode* familyNode = &vm->familyListNode;
    if (familyNode->next)
    {
        familyNode->next->prev = familyNode->prev;
    }
    if (familyNode->prev)
    {
        familyNode->prev->next = familyNode->next;
    }

    familyNode->next = NULL;
    familyNode->prev = NULL;



    if (vm >= self->fastVms && vm < (self->fastVms + 4096))
    {
        int index = vm - self->fastVms;
        self->fastVmsAlive[index] = 0; // Mark this slot as free in the allocation map.

        /*
        if (vm->specialRenderData)
        {
            free(vm->specialRenderData);
        }
        vm->specialRenderData = NULL;
        */

        // Reinitialize the VM structure for re-use rather than deleting it.
        vm->init(vm);
        return;
    }

    // Heap VM cleanup (slow path).
    if (vm)
    {
        /*
        if (vm->specialRenderData)
        {
            free(vm->specialRenderData);
        }
        vm->specialRenderData = NULL;
        */

        // free(vm);
    }
}

void AnmManager::addVm(AnmVM* vm, AnmID* outAnmId)
{
    AnmVM_listNode* newNode = &vm->globalListNode;
    newNode->entry = vm;
    vm->globalListNode.next = NULL;
    vm->globalListNode.prev = NULL;

    AnmVM_listNode* globalHead = g_anmManager->primaryGlobalHead;
    if (!globalHead)
    {
        g_anmManager->primaryGlobalTail = newNode;
    }
    else
    {
        vm->globalListNode.next = globalHead;
        globalHead->prev = newNode;
    }

    // 1-indexed?
    ++g_anmManager->id;
    if (g_anmManager->id == 0)
    {
        ++g_anmManager->id;
    }

    vm->id.id = g_anmManager->id;
    outAnmId->id = g_anmManager->id;
}

AnmLoaded* AnmManager::preloadAnm(int anmSlotIndex, const char *anmFileName)
{
    AnmManager* self = g_anmManager;

    // Return existing animation if it already loaded.
    if (self->loadedAnms[anmSlotIndex] != NULL)
    {
        fprintf(stderr, "%s: already loaded: %s\n", __func__, anmFileName);
        return self->loadedAnms[anmSlotIndex];
    }

    AnmLoaded* anmLoaded = preloadAnmFromMemory(self, anmSlotIndex, anmFileName);
    if (anmLoaded == NULL)
    {
        return NULL;
    }

    anmLoaded->anmsLoading = 1;
    // NOTE: Artificial sleeping go brrr.
    while (anmLoaded->anmsLoading != 0)
    {
        // Wait until animation loading is complete.
        Sleep(1);
    }
    fprintf(stderr, "%s: Loaded: %s\n", __func__, anmFileName);
    return anmLoaded;
}


int AnmManager::openAnmLoaded(AnmLoaded* anmLoaded, AnmHeader* anmHeader, int chunkIndex)
{
    if (anmHeader->version != 7)
    {
        fprintf(stderr, "ANM version is incorrect.\n");
        return -1;
    }

    // If it's 0, load an external texture.
    if (anmHeader->hasData == 0)
    {
        char* name = (char *) anmHeader + anmHeader->nameOffset;

        // Skip if the name has a '@' (special case).
        if (*name != '@')
        {
            char filePath[260];
            sprintf_s(filePath, "%s", name);
            uint32_t outSize;
            //uint32_t* memoryMappedFile = (uint32_t *) Platform_fileReadS(&outSize, &outSize, 1);
            u8* memoryMappedFile;
            Platform_fileRead(NULL, STR8_LIT(filePath), &memoryMappedFile, &outSize);

            if (memoryMappedFile == NULL)
            {
                fprintf(stderr, "%s: [ERROR] %s \n", __func__, name);
                return -1;
            }

            // Store the loaded file and size in the chunk data buffer.
            anmLoaded->anmLoadeds[chunkIndex].srcData = memoryMappedFile;
            anmLoaded->anmLoadeds[chunkIndex].srcDataSize = outSize;
        }
    }

    return 0;
}

AnmLoaded* AnmManager::preloadAnmFromMemory(AnmManager* self, int anmSlotIndex, const char* anmFilePath)
{
    fprintf(stderr, "%s: %s\n", __func__, anmFilePath);

    // Check if the slot index is within bounds.
    if (anmSlotIndex >= 32)
    {
        fprintf(stderr, "ANM slot index %d is out of bounds!\n", anmSlotIndex);
        return NULL;
    }

    // Resolve the file path.
    char resolvedFilePath[260];
    sprintf_s(resolvedFilePath, "%s", anmFilePath);

    // Loads an asset.
    Asset* asset = AssetLoad(resolvedFilePath);
    AnmHeader* header = (AnmHeader *) asset->data;
    if (!header)
    {
        fprintf(stderr, "ANM header is NULL!\n");
        return NULL;
    }

    // Allocate and init the AnmLoaded.
    AnmLoaded* anmLoaded = (AnmLoaded *) _MALLOC(sizeof(AnmLoaded));
    if (!anmLoaded)
    {
        fprintf(stderr, "Unable to allocate anmLoaded!\n");
        return NULL;
    }
    _MEMSET(anmLoaded, 0, sizeof(AnmLoaded));
    self->loadedAnms[anmSlotIndex] = anmLoaded;

    // Set initial fields.
    anmLoaded->anmSlotIndex = anmSlotIndex;
    anmLoaded->header = header;
    strcpy_s(anmLoaded->filePath, anmFilePath);

    // Parse the ANM header and then cound chunks, sprites, and scripts.
    AnmHeader* chunk = (AnmHeader *) header;
    int nSprites = chunk->numSprites;
    int nScripts = chunk->numScripts;
    int processedCount = 1;

    uint32_t nextOffset = chunk->nextOffset;
    AnmHeader* currentChunk = chunk;

    while (nextOffset != 0)
    {
        currentChunk = (AnmHeader *) ((char *) currentChunk + nextOffset);
        nSprites += currentChunk->numSprites;
        nScripts += currentChunk->numScripts;
        processedCount++;

        nextOffset = currentChunk->nextOffset;
    }

    anmLoaded->numAnmLoadeds = processedCount;
    anmLoaded->anmLoadeds = (TexLoaded *) _MALLOC(processedCount * sizeof(TexLoaded));
    _MEMSET(anmLoaded->anmLoadeds, 0, processedCount * sizeof(TexLoaded));

    // Keyframe data.
    anmLoaded->keyframeData = (TexLoadedSprite *) _MALLOC(nSprites * sizeof(TexLoadedSprite));

    anmLoaded->spriteData = (uint32_t *) _MALLOC(nScripts * sizeof(uint32_t));

    anmLoaded->numScripts = nScripts;
    anmLoaded->numSprites = nSprites;

    // Process each chunk. (like a linked list)
    int chunkIndex = 0;
    currentChunk = header;
    while (currentChunk != NULL)
    {
        int result = openAnmLoaded(anmLoaded, currentChunk, chunkIndex);
        if (result < 0)
        {
            fprintf(stderr, "Couldn't read ANM data.\n");
            _FREE(anmLoaded->header);
            _FREE(anmLoaded->keyframeData);
            _FREE(anmLoaded->spriteData);
            _FREE(anmLoaded);
            self->loadedAnms[anmSlotIndex] = NULL;
            return NULL;
        }

        chunkIndex++;
        if (currentChunk->nextOffset == 0)
        {
            break;
        }
        currentChunk = (AnmHeader *) ((char *) currentChunk + currentChunk->nextOffset);
    }

    return anmLoaded;
}

void AnmManager::markAnmLoadedAsReleasedVmList(AnmManager *self, AnmLoaded *anmLoaded)
{
    AnmVM_listNode* temp = self->primaryGlobalHead;
    while (temp)
    {
        AnmVM_listNode* next = temp->next;
        AnmVM* entry = temp->entry;
        temp = next;

        if (entry->anmLoaded == anmLoaded)
        {
            //entry->flags |= ANM_MARK_REMOVE;
            // TODO:
        }
    }

    temp = self->secondaryGlobalHead;
    while (temp)
    {
        AnmVM_listNode* next = temp->next;
        AnmVM* entry = temp->entry;
        temp = next;

        if (entry->anmLoaded == anmLoaded)
        {
            //entry->flags |= ANM_MARK_REMOVE;
            // TODO:
        }
    }
}

AnmVM* AnmManager::allocateVm(void)
{
    AnmManager* self = g_anmManager;
    int index = self->nextFastVmIndex;

    if (self->fastVmsAlive[index] != 0)
    {
        // Current slot is busy. Move cursor to next and try again.
        index = self->getNextFastVmIndex(index);
        self->nextFastVmIndex = index;

        // Is this valid slot also alive?
        if (self->fastVmsAlive[index] != 0)
        {
            // Both preferred slots are busy.
            // Allocate an overflow VM from the heap.
            AnmVM* vm = (AnmVM *) _MALLOC(sizeof(AnmVM));
            if (!vm)
            {
                puts("Could not allocate VM!");
                return NULL;
            }
            _MEMSET(vm, 0, sizeof(AnmVM));
            vm->spriteNumber = -1; // Inline constructor.
            AnmVM::init(vm);

            self->nextFastVmIndex = self->getNextFastVmIndex(self->nextFastVmIndex);

            return vm;
        }
    }

    AnmVM* vm = &self->fastVms[index];
    self->fastVmsAlive[index] = 1;
    self->nextFastVmIndex = self->getNextFastVmIndex(self->nextFastVmIndex);
    return vm;
}

void AnmManager::makeVmWithAnmLoaded(AnmLoaded *anmLoaded, int scriptNumber, int anmVmLayer, AnmID *idx)
{
    AnmManager* self = g_anmManager;
    // enterCriticalSection(9);
    AnmVM* vm = allocateVm();
    vm->flags |= 0x4000000;
    vm->layer = anmVmLayer;

    //vm->loadAnmScript(vm, anmLoaded, scriptNumber);
    vm->loadIntoAnmVM(vm, anmLoaded, scriptNumber);
    putInVmList(vm, idx);
    // leaveCriticalSection(9);
}

void AnmManager::releaseTextures(void)
{
    AnmManager* self = g_anmManager;
    for (int i = 0; i < N_ANM_LOADEDS; ++i)
    {
        AnmLoaded* anmLoaded = self->loadedAnms[i];
        if (anmLoaded == NULL)
        {
            continue;
        }

        TexLoaded* anmLoadedTexs = anmLoaded->anmLoadeds;
        int nAnmLoadedTexs = anmLoaded->numAnmLoadeds;
        for (int j = 0; j < nAnmLoadedTexs; ++j)
        {
            TexLoaded* entry = &anmLoadedTexs[j];
            if ((entry->flags & 1) != 0 && entry->texture != NULL)
            {
                TextureTerminate(entry, entry->texture);

                puts("Freed texture!");
            }
        }
    }
}

void AnmManager::createTextures(AnmManager* self)
{
    for (int i = 0; i < 32; ++i)
    {
        AnmLoaded* loadedAnm = self->loadedAnms[i];
        if (!loadedAnm)
        {
            continue;
        }

        if (loadedAnm->numAnmLoadeds <= 0)
        {
            continue;
        }

        for (int j = 0; j < loadedAnm->numAnmLoadeds; ++j)
        {
            TexLoaded* texLoaded = &loadedAnm->anmLoadeds[j];
            if (texLoaded->flags & 1)
            {
                int texFlags = TEXTURE_ALPHA;

                texLoaded->flags |= 1;
                AllocateMutableTexture(
                    windowHandle->w, windowHandle->h,
                    &texLoaded->texture->handle, texFlags
                );

                texLoaded->bytesPerPixel = (texFlags & TEXTURE_ALPHA) * 2 + 2;
            }
        }
    }
}

AnmVM* AnmManager::getVmById(AnmManager *self, int anmId)
{
    if (anmId == 0)
    {
        // Why is it 1-indexed?
        return NULL;
    }

    AnmVM_listNode* primaryVmList = self->primaryGlobalHead;
    while (primaryVmList)
    {
        if (primaryVmList->entry->id.id == anmId)
        {
            return primaryVmList->entry;
        }
        primaryVmList = primaryVmList->next;
    }

    AnmVM_listNode* secondaryVmList = self->secondaryGlobalHead;
    while (secondaryVmList)
    {
        if (secondaryVmList->entry->id.id == anmId)
        {
            return secondaryVmList->entry;
        }
        secondaryVmList = secondaryVmList->next;
    }

    return NULL;
}

void AnmManager::loadIntoAnmVm(AnmVM *vm, AnmLoaded *anmLoaded, int scriptNumber)
{
    AnmVM_rawInstr* instr;
    uint32_t isInit;

    if (anmLoaded->spriteData[scriptNumber] != 0 && anmLoaded->anmsLoading == 0)
    {
        vm->init(vm);
        vm->scriptNumber = static_cast<uint16_t> (scriptNumber);
        vm->flags &= 0xffffff9ff;
        vm->anmFileIndex = static_cast<uint16_t> (anmLoaded->anmSlotIndex);
        vm->anmLoaded = anmLoaded;

        // Apparently we load it from the sprite data (the first entry).
        instr = (AnmVM_rawInstr *) &anmLoaded->spriteData[scriptNumber];
        vm->startOfScript = instr;
        vm->currentInstr = instr;
        vm->timeInScript.set(&vm->timeInScript, 0);
        vm->flags &= 0xffffffffffe;

        // Run instructions already!
        vm->run(vm);
        g_anmManager->allocatedVmCountMaybe++;

        return;
    }

    memset(vm, 0, sizeof(AnmVM));
    puts("OK");
}

void AnmManager::putInVmList(AnmVM *vm, AnmID *idx)
{
    AnmManager* self = g_anmManager;
    AnmVM_listNode* curVm = &vm->globalListNode;
    curVm->entry = vm;
    vm->globalListNode.next = NULL;
    vm->globalListNode.prev = NULL;
    if (self->primaryGlobalHead == NULL)
    {
        self->primaryGlobalHead = curVm;
    }
    else
    {
        AnmVM_listNode* primaryVms = self->primaryGlobalTail;
        if (primaryVms->next != NULL)
        {
            (vm->globalListNode).next = primaryVms->next;
            primaryVms->next->prev = curVm;
        }
        primaryVms->next = curVm;
        vm->globalListNode.prev = primaryVms;
    }
    self->primaryGlobalTail = curVm;

    // Strange but correct game logic.
    self->id++;
    if (self->id == 0)
    {
        self->id++;
    }

    vm->id.id = self->id;
    idx->id = self->id;
}

void AnmManager::spawnVmAtPosition(AnmLoaded *anmLoaded, uint32_t scriptNumber, int layer, AnmID *outAnmId)
{
    // enterCriticalSection(9);
    AnmVM* vm = allocateVm();
    vm->flags |= 0x4000000;
    vm->layer = layer;
    //vm->loadAnmScript(vm, anmLoaded, scriptNumber);
    vm->loadIntoAnmVM(vm, anmLoaded, scriptNumber);
    addVm(vm, outAnmId);
    // leaveCriticalSeciton(9)
}

void AnmManager::releaseAnmLoaded(AnmManager* self, AnmLoaded* anmLoaded)
{
    if (!anmLoaded->header)
    {
        return;
    }

    self->markAnmLoadedAsReleasedVmList(self, anmLoaded);

    if (anmLoaded->numAnmLoadeds > 0)
    {
        for (int i = 0; i < anmLoaded->numAnmLoadeds; ++i)
        {
            TexLoaded* entry = &anmLoaded->anmLoadeds[i];
            if (entry->texture)
            {
                TextureTerminate(entry, entry->texture);
                // TODO: Release texture.
                //entry->texture-
            }
            if (entry->srcData)
            {
                free(entry->srcData);
                entry->srcData = NULL;
            }
        }
    }

    if (anmLoaded->anmLoadeds)
    {
        free(anmLoaded->anmLoadeds);
        anmLoaded->anmLoadeds = NULL;
    }

    if (anmLoaded->keyframeData)
    {
        free(anmLoaded->keyframeData);
        anmLoaded->keyframeData = NULL;
    }

    // And then free the header.
    if (anmLoaded->header)
    {
        free(anmLoaded->header);
        anmLoaded->header = NULL;
    }
}


void AnmManager::setInterruptById(int vmId, uint16_t pendingInterruptFlag)
{
    AnmVM* vm = getVmById(g_anmManager, vmId);
    AnmVM_listNode* vmList = vm->familyListNode.prev;
    vm->pendingInterrupt = pendingInterruptFlag;

    // Apply this to inherited VMs.
    if (vm && vmList == NULL)
    {
        for (vmList = (vm->familyListNode).next; vmList != NULL; vmList = vmList->next)
        {
            vmList->entry->pendingInterrupt = pendingInterruptFlag;
        }
    }
}

void AnmManager::setVmPosition(AnmVM* vm, vec3* const position)
{
    vm->entityPos.x = position->x;
    vm->entityPos.y = position->y;
    vm->entityPos.z = position->z;

    AnmVM_listNode* node = vm->familyListNode.prev;
    if (!node)
    {
        node = vm->familyListNode.next;
        while (node)
        {
            AnmVM* entry = node->entry;
            node = node->next;
            entry->entityPos.x = position->x;
            entry->entityPos.y = position->y;
            entry->entityPos.z = position->z;
        }
    }
}

void AnmManager::setPositionById(int vmId, vec3* const position)
{
    AnmVM* vm = getVmById(g_anmManager, vmId);
    if (!vm)
    {
        return;
    }

    AnmVM_listNode* node = vm->familyListNode.prev;
    if (!node)
    {
        node = vm->familyListNode.next;
        while (node)
        {
            AnmVM* entry = node->entry;
            node = node->next;
            entry->entityPos.x = position->x;
            entry->entityPos.y = position->y;
            entry->entityPos.z = position->z;
        }
    }
}

/*
   VM drawing.
*/

void AnmManager::drawVm(AnmManager *self, AnmVM *vm)
{
    // TODO:
    puts("NOT IMPLEMENTED");
}

void AnmManager::renderLayer(AnmManager* self, int layer)
{
    // enterCriticalSection(9);

    AnmVM* vm = &self->vmLayers[layer];
    while (vm->nextInLayerList)
    {
        // We can draw with layers?
        if ((vm->flags & 0x4000000) == 0)
        {
            if (vm->onDraw)
            {
                vm->onDraw(vm);
            }
            drawVm(self, vm);
        }
        vm = vm->nextInLayerList;
    }

    // leaveCriticalSection(9);
}

void AnmManager::onTick(void* args)
{
    AnmManager* self = reinterpret_cast<AnmManager *>(args);

    int vmLayer;
    AnmVM* vmLayers[33]{};
    AnmVM_listNode* list;
    AnmVM* vm;

    // enterCriticalSection(9);

    // Initialize layers.
    vmLayer = 0;
    vm = self->vmLayers;
    do
    {
        vmLayers[vmLayer] = vm;
        vm->nextInLayerList = (AnmVM *) NULL;
        vmLayer = vmLayer + 1;
        vm = vm + 1;
    } while (vmLayer < 0x1d);


    list = self->primaryGlobalHead;
    while (list)
    {
        vm = list->entry;
        list = list->next;
        if ((vm->flags & 0x400000) == 0)
        {
            // ChainCallbackResult result;
            if (vm->onTick)
            {
                //result = vm->onTick;
                vm->onTick(vm);
            }

            // TODO: AnmVM::run(vm).
            AnmVM::run(vm);

            if (vm->onTick)// && result == ChainCallbackResult::continueAndRemoveJob)
            {
                vmLayers[vm->layer]->nextInLayerList = vm;
                vmLayers[vm->layer] = vm;
                vm->nextInLayerList = NULL;
            }
            else
            {
                removeVm(self, vm);
            }
        }
        else
        {
            removeVm(self, vm);
        }

        self->someTickCounter++;
    }

    // leaveCriticalSection(9);
}


