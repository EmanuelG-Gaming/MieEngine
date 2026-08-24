#ifndef WDOW_H_
#define WDOW_H_ 1

#include "platform.h"
#include "../base/base_defs.h"

// Some compile-time configs.
#define FULLSCREEN 0
//#define VSYNC_ENABLED 1

typedef enum WINFLAGS {
    WIN_FLAG_NONE = 0,
    WIN_FLAG_SHOULD_CLOSE = (1 << 0),
} WINFLAGS;

typedef struct WINBACKEND WINBACKEND;
typedef struct WINDOW {
    const wchar_t* title;
    int w, h;
    u32 flags;
    u32 dpi, rawDpi;
    WINBACKEND* backend;
} WINDOW;

// TODO: move this somewhere else.
typedef struct GRAPHICS {
    int frameWidth, frameHeight;
    float aspect;
} GRAPHICS;

extern WINDOW* windowHandle;
extern GRAPHICS graphics;


/*
   The Window.
*/
extern int PlatformInit(void);

extern WINDOW* WindowInit(const wchar_t* title, int width, int height);
extern void WindowTerminate(WINDOW* win);

extern void WindowProcessEvents(WINDOW* win);
extern int WindowOpened(WINDOW* win);

/*
   Misc.
*/
extern void PlatformSleepMs(unsigned int ms);
extern u64 PlatformTimeUsec(void);


#endif /* WDOW_H_ */
