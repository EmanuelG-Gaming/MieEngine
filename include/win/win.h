#include "win/events.h"
#ifndef WIN_H_
#define WIN_H_ 1

#include "base/base_defs.h"
#include "base/math/mathf.h"
#include "events.h"

//#include "../mem/arena.h"


#define MOUSE_LEFT (0)
#define MOUSE_MIDDLE (1)
#define MOUSE_RIGHT (2)
#define MOUSE_MAX (3)

#define WIN_FLAG_SHOULD_CLOSE (1)

typedef struct WinBackend WinBackend;

typedef struct Input {
    /*
    float mouseXPos;
    float mouseYPos;

    float mouseScrollX;
    float mouseScrollY;
    */

    float currentScrollX;

    vec2 mouseCurrentPos;
    b8 mouseCurrentButtons[MB_MOUSE_COUNT];
    b8 currentKeys[M_KEY_COUNT];

    vec2 mousePreviousPos;
    b8 mousePreviousButtons[MB_MOUSE_COUNT];
    b8 previousKeys[M_KEY_COUNT];

    // Putting this in an event queue.
    WindowEvent* firstEvent;
    WindowEvent* lastEvent;

    //int keys[256];
    //int mouseButtons[MOUSE_MAX];
} Input;

typedef struct Window {
    const wchar_t* title;

    u32 w, h;
    u32 flags;

    WinBackend* backend;
    void* user;
} Window;

extern Window* windowHandle;
extern Input input;


extern Window* WindowInit(const wchar_t* title, int width, int height);
extern void WindowTerminate(Window* win);

extern void WindowProcessEvents(Window* win);
extern int WindowOpened(Window* win);


/*
   Input.
*/

extern int IsKeyDown(unsigned int key);
extern int IsKeyHeld(unsigned int key);
extern int IsKeyUp(unsigned int key);

extern int IsMouseButtonDown(unsigned int button);
extern int IsMouseButtonHeld(unsigned int button);
extern int IsMouseButtonUp(unsigned int button);


extern float GetMouseXPosition(void);
extern float GetMouseYPosition(void);



#endif /* WIN_H_ */
