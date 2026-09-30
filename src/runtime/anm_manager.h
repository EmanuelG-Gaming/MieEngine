#ifndef ANM_MANAGER_H_
#define ANM_MANAGER_H_ 1

#include "anm_vm.h"

/*
   4096 "fast" pre-allocated VMs.
   32 textures that can be used at once.

   -> Probably the VMs have to be less dependent on heap allocations/use a more general-purpose interface for allocating memory.
*/


class AnmManager {
public:
    AnmVM fastVms[4096];
    uint8_t fastVmsAlive[4096];
    int nextFastVmIndex;

    AnmVM primaryVm;

    AnmVM_listNode* primaryGlobalHead;
    AnmVM_listNode* primaryGlobalTail;

    AnmVM_listNode* secondaryGlobalHead;
    AnmVM_listNode* secondaryGlobalTail;

    AnmLoaded* loadedAnms[32];

    AnmVM vmLayers[31];
    int id;

    int allocatedVmCountMaybe;
    int someTickCounter;

    AnmManager();
    ~AnmManager();

    static void drawSprite2D(AnmManager* self, uint32_t layer, AnmVM* vm);

    static void drawVm(AnmManager* self, AnmVM* vm);

    static void addVm(AnmVM* vm, AnmID* outId);
    static void removeVm(AnmManager* self, AnmVM* vm);

    static AnmLoaded* preloadAnm(int anmSlotIndex, const char* anmFileName);
    static int openAnmLoaded(AnmLoaded* anmLoaded, AnmHeader* anmHeader, int chunkIndex);
    static AnmLoaded* preloadAnmFromMemory(AnmManager* self, int anmSlotIndex, const char* anmFilePath);

    static void putInVmList(AnmVM* vm, AnmID* idx);
    static void markAnmLoadedAsReleasedVmList(AnmManager* self, AnmLoaded* anmLoaded);

    static void makeVmWithAnmLoaded(AnmLoaded* anmLoaded, int scriptNumber, int anmVmLayer, AnmID* idx);

    static void setVmPosition(AnmVM* vm, vec3* const position);

    static void setInterruptById(int vmId, uint16_t pendingInterruptFlag);
    static void setPositionById(int vmId, vec3* const position);


    static AnmVM* allocateVm(void);
    static AnmManager* init(AnmManager* self);
    static AnmVM* getVmById(AnmManager* self, int anmId);

    static void loadIntoAnmVm(AnmVM* vm, AnmLoaded* anmLoaded, int scriptNumber);
    static void spawnVmAtPosition(AnmLoaded* anmLoaded, uint32_t scriptNumber, int layer, AnmID* outAnmId);

    void createTextures(AnmManager* self);
    static void releaseTextures(void);

    void releaseAnmLoaded(AnmManager* self, AnmLoaded* anmLoaded);

    // The update functions.
    void renderLayer(AnmManager* self, int layer);
    static void onTick(void* args);

private:
    static constexpr int N_FAST_VMS = 4096;
    static constexpr int N_ANM_LOADEDS = 32;

    static inline int getNextFastVmIndex(int current)
    {
        return (current & (N_FAST_VMS - 1));
    }
};
extern AnmManager* g_anmManager;

#endif /* ANM_MANAGER_H_ */
