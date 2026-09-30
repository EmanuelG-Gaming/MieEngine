#include "../audio.h"
#include "../../base/base_log.h"
#include "../../win/win.h"
#include "../../mem/arena.h"

#include <windows.h>
// For .wav file loading.
#include <mmsystem.h>
#include <dsound.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")


static LPDIRECTSOUND directSound = NULL;



extern AUD_sound* AudioLoadSound_wav(ARENA* arena, const char* filePath)
{
    WAVEFORMATEX wfx = { 0 };
    HMMIO hFile = mmioOpenW((LPWSTR) L"assets/csfteow.wav", NULL, MMIO_READ);
    if (!hFile)
    {
        LogErrorEmitF("%s: Failed to open .wav file at path: %s\n", __func__, filePath);
        directSound->Release();
        return NULL;
    }

    // Riff file.
    MMCKINFO ckRiff = { 0 };
    ckRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioDescend(hFile, &ckRiff, NULL, MMIO_FINDRIFF))
    {
        LogErrorEmitF("%s: Not a valid WAV file. %s\n", __func__, filePath);
        mmioClose(hFile, 0);
        directSound->Release();
        return NULL;
    }

    MMCKINFO ckFormat = { 0 };
    ckFormat.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hFile, &ckFormat, &ckRiff, MMIO_FINDCHUNK))
    {
        LogErrorEmitF("%s: Failed to find format chunk! %s\n", __func__, filePath);
        mmioClose(hFile, 0);
        directSound->Release();
        return NULL;
    }

    DWORD formatSize = ckFormat.cksize;
    if (mmioRead(hFile, (HPSTR) &wfx, formatSize) != (LONG) formatSize)
    {
        LogErrorEmitF("%s: Failed to read format chunk. %s\n", __func__, filePath);
        mmioClose(hFile, 0);
        directSound->Release();
        return NULL;
    }

    mmioAscend(hFile, &ckFormat, 0);

    MMCKINFO ckData = { 0 };
    ckData.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hFile, &ckData, &ckRiff, MMIO_FINDCHUNK))
    {
        LogErrorEmitF("%s: Failed to find data chunk! %s\n", __func__, filePath);
        mmioClose(hFile, 0);
        directSound->Release();
        return NULL;
    }

    // Now we load the wave data file.
    ARENA_TEMP maybeTemp = ArenaTempBegin(arena);

    DWORD dataSize = ckData.cksize;
    BYTE* waveData = ArenaPushArrayNZ(maybeTemp.arena, BYTE, dataSize);
    if (mmioRead(hFile, (HPSTR) waveData, dataSize) != (LONG) dataSize)
    {
        LogErrorEmitF("%s: Failed to read data chunk! %s\n", __func__, filePath);
        ArenaTempEnd(maybeTemp);

        mmioClose(hFile, 0);
        directSound->Release();
        return NULL;
    }
    mmioClose(hFile, 0);

    DSBUFFERDESC bd = { 0 };
    bd.dwSize = sizeof(DSBUFFERDESC);
    // We can control the volume, frequency and panning of the sound.
    bd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN;
    bd.dwBufferBytes = dataSize;
    bd.lpwfxFormat = &wfx;

    IDirectSoundBuffer* pbuffer = NULL;
    HRESULT res = directSound->CreateSoundBuffer(&bd, &pbuffer, NULL);
    if (FAILED(res))
    {
        LogErrorEmitF("%s: Failed to create DSound buffer!\n", __func__);
        ArenaTempEnd(maybeTemp);
        directSound->Release();

        return NULL;
    }

    LPVOID audioPtr1 = NULL;
    DWORD audioBytes1 = 0;
    LPVOID audioPtr2 = NULL;
    DWORD audioBytes2 = 0;

    res = pbuffer->Lock(0, dataSize, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);
    if (FAILED(res))
    {
        LogErrorEmitF("%s: Failed to lock buffer!\n", __func__);
        pbuffer->Release();
        ArenaTempEnd(maybeTemp);
        directSound->Release();

        return NULL;
    }

    // And then copy some of our data.
    _MEMCPY(audioPtr1, waveData, audioBytes1);
    if (audioPtr2)
    {
        _MEMCPY(audioPtr2, waveData + audioBytes1, audioBytes2);
    }

    pbuffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);

    ArenaTempEnd(maybeTemp);

    AUD_sound* sound = ArenaPushStruct(arena, AUD_sound);
    sound->user = pbuffer;

    return sound;
}

extern void AudioSoundTerminate(AUD_sound *sound)
{
    LPDIRECTSOUNDBUFFER buf = (LPDIRECTSOUNDBUFFER) (sound->user);
    buf->Release();
}

extern void AudioSoundPlay(AUD_sound* sound)
{
    LPDIRECTSOUNDBUFFER buf = (LPDIRECTSOUNDBUFFER) (sound->user);
    buf->SetCurrentPosition(0);
    buf->Play(0, 0, 0);
}



static HRESULT DSoundInit(Window* win)
{
    HWND winHandle = *((HWND *) win->user);

    HRESULT res = DirectSoundCreate(&DSDEVID_DefaultPlayback, &directSound, NULL);
    if (FAILED(res))
    {
        LogErrorEmitF("%s: Failed to init DirectSound!\n", __func__);
        return res;
    }

    res = directSound->SetCooperativeLevel(winHandle, DSSCL_PRIORITY);
    if (FAILED(res))
    {
        LogErrorEmitF("%s: Failed to set cooperative level!\n", __func__);
        directSound->Release();
        return res;
    }

    return S_OK;
}


// Get window handle for some reason.
extern void AudioInit(Window* window)
{
    if (FAILED(DSoundInit(window)))
    {
        LogErrorEmitF("%s: Failed to load DirectSound!\n", __func__);
        return;
    }
}

extern void AudioTerminate(void)
{
    directSound->Release();
}
