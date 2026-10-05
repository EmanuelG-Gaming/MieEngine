#ifndef WIN_H_
#define WIN_H_ 1

#include "../base/base_defs.h"
#include "../mem/arena.h"


#define MOUSE_LEFT (0)
#define MOUSE_MIDDLE (1)
#define MOUSE_RIGHT (2)
#define MOUSE_MAX (3)

#define WIN_FLAG_SHOULD_CLOSE (1)

typedef struct WinBackend WinBackend;

typedef struct Input {
    float mouseXPos;
    float mouseYPos;

    float mouseScrollX;
    float mouseScrollY;

    int keys[256];
    int mouseButtons[MOUSE_MAX];
} Input;

typedef struct Window {
    const wchar_t* title;

    WinBackend* backend;
    void* user;

    u32 w, h;

    u32 flags;
} Window;

extern Window* windowHandle;
extern Input input;


extern Window* WindowInit(ARENA* arena, const wchar_t* title, int width, int height);
extern void WindowTerminate(Window* win);

extern void WindowProcessEvents(Window* win);
extern int WindowOpened(Window* win);


/*
   Input.
*/

extern int DoKeyDown(unsigned int key);
extern int DoKeyUp(unsigned int key);


extern int IsKeyDown(unsigned int key);

extern int SetMouseXPosition(float x);
extern int SetMouseYPosition(float y);
extern float GetMouseXPosition(void);
extern float GetMouseYPosition(void);




#endif /* WIN_H_ */
