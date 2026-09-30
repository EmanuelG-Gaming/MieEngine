#ifndef AUDIO_H_
#define AUDIO_H_ 1


#include "../mem/arena.h"
#include "../win/win.h"

#define HUMAN_HZ 44.1

typedef struct AUD_sound {
    // User data buffer.
    void* user;
} AUD_sound;

extern float audioVolume;
extern float musicVolume;
extern float soundVolume;


extern AUD_sound* AudioLoadSound_wav(ARENA* arena, const char* filePath);
extern void AudioSoundTerminate(AUD_sound* sound);

extern void AudioSoundPlay(AUD_sound* sound);

extern void AudioInit(Window* window);
extern void AudioTerminate(void);

#endif /* AUDIO_H_ */
