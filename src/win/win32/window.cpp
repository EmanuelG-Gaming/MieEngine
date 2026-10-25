/*
   TODO: You have to define these
   compile-time args BEFORE you #include <windows.h>.
*/
#include "base/base_defs.h"
#include "win/events.h"
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <minwinbase.h>

//#include <d3d11.h>
//#include <d3dcompiler.h>
#endif /* _WIN32 */

#include "win/win.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>



/*
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
*/

#include "platform.h"

#include <fileapi.h>
#include <windef.h>
#include <windows.h>
#include <winnt.h>
#include <winuser.h>

#include "platform/platform.h"

#include "base/base_log.h"

#define WIN_CLASS_NAME L"CustomWindow"

typedef struct WinBackend {
    HINSTANCE hinstance;
    HWND hwnd;
} WinBackend;

/*
   Global Window Handle.
*/

Window* windowHandle = NULL;
Input input;

static b32 platformInitialized = FALSE;
static b32 windowInitialized = FALSE;

static Key_t keymap[256];

static void InitKeymap(void);


#define GET_MOUSE_VEC2_WIN32(lparam) \
    CLITERAL(vec2) { \
    (float) (lparam & 0xFFFF), \
    (float) ((lparam >> 16) & 0xFFFF), \
    }


static int RegisterWinClass(void);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);


extern Window* WindowInit(const wchar_t* title, int width, int height)
{
    PlatformInit();

    //LogInfo("Initializing window!\n");

    if (!windowInitialized)
    {
        InitKeymap();

        windowInitialized = RegisterWinClass();
        puts("Init window class!");
    }

    if (!windowInitialized)
    {
        // If it was still not initialized.
        // TODO: Do log call?
        LogErrorEmitF("%s: Failed to initialize window!\n", __func__);
        return NULL;
    }

    //ARENA_TEMP arenaTemp = ArenaTempBegin(arena);


    //Window* win = ArenaPushStruct(arenaTemp.arena, Window);
    Window* win = (Window *) _MALLOC(sizeof(Window));
    _MEMSET(win, 0, sizeof(Window));

    win->title = static_cast<const wchar_t *> (title);
    win->w = width;
    win->h = height;
    win->flags = 0;


    //win->backend = ArenaPushStruct(arenaTemp.arena, WinBackend);
    win->backend = (WinBackend *) _MALLOC(sizeof(WinBackend));
    _MEMSET(win->backend, 0, sizeof(WinBackend));

    RECT winRect = { 0, 0, (int) width, (int) height };
    if (!AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, FALSE))
    {
        // TODO: Print call?
        LogErrorEmitF("%s: Failed to adjust window rect!\n", __func__);
        goto fail;
    }

    win->backend->hwnd = CreateWindowW(
        WIN_CLASS_NAME,
        win->title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winRect.right - winRect.left, winRect.bottom - winRect.top,
        NULL, NULL, NULL, NULL
    );
    if (win->backend->hwnd == NULL)
    {
        // Logging?
        LogErrorEmitF("%s: Failed to init window!\n", __func__);
        return NULL;
    }

    // We add the user data, i.e. the window handle that
    // can be accessed within other parts of the code.
    win->user = win->backend->hwnd;

    SetWindowLongPtrW(win->backend->hwnd, GWLP_USERDATA, (LONG_PTR) win);

    // Init input.
    {
        _MEMSET(&input, 0, sizeof(input));

        //input.mouseXPos = 0;
        //input.mouseYPos = 0;
        /*
        // Initialize all the keys.
        for (int i = 0; i < 256; ++i)
        {
            input.keys[i] = FALSE;
        }
        */
    }

    // Equip GFX.

    ShowWindow(win->backend->hwnd, SW_SHOW);

    windowHandle = win;

    //ArenaTempEnd(arenaTemp);

    return win;

fail:
    if (win->backend->hwnd != NULL)
    {
        DestroyWindow(win->backend->hwnd);
    }
    //ArenaTempEnd(arenaTemp);
    return NULL;
}

extern void WindowTerminate(Window* win)
{
    if (win == NULL)
    {
        return;
    }

    ShowCursor(TRUE);

    /*
    if (FULLSCREEN)
    {
        ChangeDisplaySettings(NULL, 0);
    }
    */

    DestroyWindow(win->backend->hwnd);
    win->backend->hwnd = NULL;

    UnregisterClass(win->title, win->backend->hinstance);
    win->backend->hinstance = NULL;

    // Remove backend.
    if (win->backend)
    {
        // _FREE(win->backend);
        win->backend = NULL;
    }

    //PlatformTerminate();
}


extern void WindowProcessEvents(Window* win)
{
    // Linked list of events.
    input.firstEvent = NULL;
    input.lastEvent = NULL;

    input.mousePreviousPos = input.mouseCurrentPos;

    _MEMCPY(input.mousePreviousButtons,
            input.mouseCurrentButtons,
            sizeof(MouseButton_t) * MB_MOUSE_COUNT);

    _MEMCPY(input.previousKeys,
            input.currentKeys,
            sizeof(Key_t) * M_KEY_COUNT);


    MSG msg = { 0 };
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}



/*
   Input.
*/

static void WinProcessMouseDown(LPARAM lparam, Window* win,
    WindowEvent* event, MouseButton_t mb)
{
    assert(mb < MB_MOUSE_COUNT);

    vec2 pos = GET_MOUSE_VEC2_WIN32(lparam);

    input.mouseCurrentPos = pos;
    input.mouseCurrentButtons[mb] = TRUE;

    WindowEvent e { 0 };
    e.type = WIN_EVENT_MOUSE_DOWN;
    e.mouseDown = CLITERAL(EventMouseDown) { pos, mb };
    *event = e;
}

static void WinProcessMouseUp(LPARAM lparam, Window* win,
    WindowEvent* event, MouseButton_t mb)
{
    assert(mb < MB_MOUSE_COUNT);

    vec2 pos = GET_MOUSE_VEC2_WIN32(lparam);

    input.mouseCurrentPos = pos;
    input.mouseCurrentButtons[mb] = FALSE;

    WindowEvent e { 0 };
    e.type = WIN_EVENT_MOUSE_UP;
    e.mouseUp = CLITERAL(EventMouseUp) { pos, mb };
    *event = e;
}




extern int IsKeyDown(unsigned int key)
{
    return (input.previousKeys[key] == FALSE) && (input.currentKeys[key] == TRUE);
}
extern int IsKeyHeld(unsigned int key)
{
    return input.currentKeys[key];
}
extern int IsKeyUp(unsigned int key)
{
    return (input.previousKeys[key] == TRUE) && (input.currentKeys[key] == FALSE);
}


extern int IsMouseButtonDown(unsigned int button)
{
    return (input.mousePreviousButtons[button] == FALSE) && (input.mouseCurrentButtons[button] == TRUE);
}
extern int IsMouseButtonHeld(unsigned int button)
{
    return input.mouseCurrentButtons[button];
}
extern int IsMouseButtonUp(unsigned int button)
{
    return (input.mousePreviousButtons[button] == TRUE) && (input.mouseCurrentButtons[button] == FALSE);
}

extern float GetMouseXPosition(void)
{
    return input.mouseCurrentPos.x;
}
extern float GetMouseYPosition(void)
{
    return input.mouseCurrentPos.y;
}







/*
   Window.
*/

extern int WindowOpened(Window* win)
{
    return (win->flags & WIN_FLAG_SHOULD_CLOSE) == 0;
}


static int RegisterWinClass(void)
{
    HINSTANCE moduleHandle = GetModuleHandleW(NULL);
    if (moduleHandle == NULL)
    {
        // TODO: Do logging?
        LogErrorEmitF("%s: Failed to get module handle!\n", __func__);
        return FALSE;
    }
    
    WNDCLASSEX wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = moduleHandle;
    wc.hIcon = LoadIcon(wc.hInstance, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WIN_CLASS_NAME;
    wc.cbSize = sizeof(WNDCLASSEX);


    ATOM atom = RegisterClassEx(&wc);
    if (!atom)
    {
        LogErrorEmitF("%s: Failed to register window class!\n", __func__);
        return FALSE;
    }

    return TRUE;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    Window* win = (Window *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (win == NULL)
    {
        return DefWindowProcW(hwnd, umsg, wparam, lparam);
    }

    b32 postEvent = FALSE;
    b32 returnDef = FALSE;

    WindowEvent dummy { 0 };
    WindowEvent* event = &dummy;

    switch (umsg)
    {
        case WM_MOUSEMOVE:
        {
            vec2 pos = GET_MOUSE_VEC2_WIN32(lparam);

            input.mouseCurrentPos = pos;

            WindowEvent g { 0 };
            g.type = WIN_EVENT_MOUSE_MOVE;
            g.mouseMove = CLITERAL(EventMouseMove) {
                pos,
            };
            *event = g;
        } break;

        case WM_MOUSEHWHEEL:
        {
            int deltaUnscaled = (int) GET_WHEEL_DELTA_WPARAM(wparam);
            float deltaX = (float) deltaUnscaled / (float) WHEEL_DELTA;

            input.currentScrollX += deltaX;

            WindowEvent g { 0 };
            g.type = WIN_EVENT_SCROLL;
            g.scroll = CLITERAL(EventScroll) {
                deltaX,
            };
            *event = g;
        } break;

        // Check if a key has been pressed on the keyboard.
        case WM_KEYDOWN:
        {
            Key_t key = wparam <= STATIC_ARR_LEN(keymap) ? keymap[wparam] : M_KEY_NONE;

            input.currentKeys[key] = TRUE;

            WindowEvent g { 0 };
            g.type = WIN_EVENT_KEY_DOWN;
            g.keyDown = CLITERAL(EventKeyDown) {
                key,
            };
            *event = g;
        } break;

        case WM_KEYUP:
        {
            Key_t key = wparam <= STATIC_ARR_LEN(keymap) ? keymap[wparam] : M_KEY_NONE;

            input.currentKeys[key] = FALSE;

            WindowEvent g { 0 };
            g.type = WIN_EVENT_KEY_UP;
            g.keyUp = CLITERAL(EventKeyUp) {
                key,
            };
            *event = g;
        } break;

        // Left, middle, right.
        case WM_LBUTTONDOWN: { WinProcessMouseDown(lparam, win, event, MB_MOUSE_LEFT ); } break;
        case WM_LBUTTONUP: { WinProcessMouseUp(lparam, win, event, MB_MOUSE_LEFT ); } break;
        case WM_MBUTTONDOWN: { WinProcessMouseDown(lparam, win, event, MB_MOUSE_MIDDLE ); } break;
        case WM_MBUTTONUP: { WinProcessMouseUp(lparam, win, event, MB_MOUSE_MIDDLE ); } break;
        case WM_RBUTTONDOWN: { WinProcessMouseDown(lparam, win, event, MB_MOUSE_RIGHT ); } break;
        case WM_RBUTTONUP: { WinProcessMouseUp(lparam, win, event, MB_MOUSE_RIGHT ); } break;


        case WM_SIZE:
        {
            // Don't post events.
            postEvent = FALSE;

            u32 w = (u32) LOWORD(lparam);
            u32 h = (u32) HIWORD(lparam);

            win->w = w;
            win->h = h;

        } break;

        case WM_CLOSE:
        {
            postEvent = FALSE;

            win->flags |= WIN_FLAG_SHOULD_CLOSE;
        } break;

        default:
        {
            return DefWindowProc(hwnd, umsg, wparam, lparam);
        } break;
    }

    /*
    if (postEvent)
    {
        SLL_APPEND_BACK(input.firstEvent, input.lastEvent, event);
    }
    else if (tmpArena != NULL)
    {
    }
    */



    return returnDef ? DefWindowProcW(hwnd, umsg, wparam, lparam) : 0;
}

static void InitKeymap(void)
{
    keymap[VK_BACK] = M_KEY_BACKSPACE;
    keymap[VK_TAB] = M_KEY_TAB;
    keymap[VK_RETURN] = M_KEY_ENTER;
    keymap[VK_CAPITAL] = M_KEY_CAPSLOCK;
    keymap[VK_ESCAPE] = M_KEY_ESCAPE;
    keymap[VK_SPACE] = M_KEY_SPACE;
    keymap[VK_PRIOR] = M_KEY_PAGEUP;
    keymap[VK_NEXT] = M_KEY_PAGEDOWN;
    keymap[VK_END] = M_KEY_END;
    keymap[VK_HOME] = M_KEY_HOME;
    keymap[VK_LEFT] = M_KEY_ARROW_LEFT;
    keymap[VK_UP] = M_KEY_ARROW_UP;
    keymap[VK_RIGHT] = M_KEY_ARROW_RIGHT;
    keymap[VK_DOWN] = M_KEY_ARROW_DOWN;
    keymap[VK_INSERT] = M_KEY_INSERT;
    keymap[VK_DELETE] = M_KEY_DELETE;


    // 1-to-1 mapping.
    keymap['0'] = M_KEY_0;
    keymap['1'] = M_KEY_1;
    keymap['2'] = M_KEY_2;
    keymap['3'] = M_KEY_3;
    keymap['4'] = M_KEY_4;
    keymap['5'] = M_KEY_5;
    keymap['6'] = M_KEY_6;
    keymap['7'] = M_KEY_7;
    keymap['8'] = M_KEY_8;
    keymap['9'] = M_KEY_9;
    keymap['A'] = M_KEY_A;
    keymap['B'] = M_KEY_B;
    keymap['C'] = M_KEY_C;
    keymap['D'] = M_KEY_D;
    keymap['E'] = M_KEY_E;
    keymap['F'] = M_KEY_F;
    keymap['G'] = M_KEY_G;
    keymap['H'] = M_KEY_H;
    keymap['I'] = M_KEY_I;
    keymap['J'] = M_KEY_J;
    keymap['K'] = M_KEY_K;
    keymap['L'] = M_KEY_L;
    keymap['M'] = M_KEY_M;
    keymap['N'] = M_KEY_N;
    keymap['O'] = M_KEY_O;
    keymap['P'] = M_KEY_P;
    keymap['Q'] = M_KEY_Q;
    keymap['R'] = M_KEY_R;
    keymap['S'] = M_KEY_S;
    keymap['T'] = M_KEY_T;
    keymap['U'] = M_KEY_U;
    keymap['V'] = M_KEY_V;
    keymap['W'] = M_KEY_W;
    keymap['X'] = M_KEY_X;
    keymap['Y'] = M_KEY_Y;
    keymap['Z'] = M_KEY_Z;

    keymap[VK_NUMPAD0] = M_KEY_NUMPAD_0;
    keymap[VK_NUMPAD1] = M_KEY_NUMPAD_1;
    keymap[VK_NUMPAD2] = M_KEY_NUMPAD_2;
    keymap[VK_NUMPAD3] = M_KEY_NUMPAD_3;
    keymap[VK_NUMPAD4] = M_KEY_NUMPAD_4;
    keymap[VK_NUMPAD5] = M_KEY_NUMPAD_5;
    keymap[VK_NUMPAD6] = M_KEY_NUMPAD_6;
    keymap[VK_NUMPAD7] = M_KEY_NUMPAD_7;
    keymap[VK_NUMPAD8] = M_KEY_NUMPAD_8;
    keymap[VK_NUMPAD9] = M_KEY_NUMPAD_9;

    keymap[VK_MULTIPLY] = M_KEY_NUMPAD_MULTIPLY;
    keymap[VK_ADD] = M_KEY_NUMPAD_ADD;
    keymap[VK_SUBTRACT] = M_KEY_NUMPAD_SUBTRACT;
    keymap[VK_DECIMAL] = M_KEY_NUMPAD_DECIMAL;
    keymap[VK_DIVIDE] = M_KEY_NUMPAD_DIVIDE;

    keymap[VK_F1] = M_KEY_F1;
    keymap[VK_F2] = M_KEY_F2;
    keymap[VK_F3] = M_KEY_F3;
    keymap[VK_F4] = M_KEY_F4;
    keymap[VK_F5] = M_KEY_F5;
    keymap[VK_F6] = M_KEY_F6;
    keymap[VK_F7] = M_KEY_F7;
    keymap[VK_F8] = M_KEY_F8;
    keymap[VK_F9] = M_KEY_F9;
    keymap[VK_F10] = M_KEY_F10;
    keymap[VK_F11] = M_KEY_F11;
    keymap[VK_F12] = M_KEY_F12;

    keymap[VK_NUMLOCK] = M_KEY_NUM_LOCK;
    keymap[VK_SCROLL] = M_KEY_SCROLL_LOCK;
    keymap[VK_SHIFT] = M_KEY_SHIFT;
    keymap[VK_CONTROL] = M_KEY_CONTROL;
    keymap[VK_MENU] = M_KEY_ALT;
    keymap[VK_LSHIFT] = M_KEY_SHIFT;
    keymap[VK_RSHIFT] = M_KEY_SHIFT;
    keymap[VK_LCONTROL] = M_KEY_CONTROL;
    keymap[VK_RCONTROL] = M_KEY_CONTROL;
    keymap[VK_LMENU] = M_KEY_ALT;
    keymap[VK_RMENU] = M_KEY_ALT;
    keymap[VK_OEM_1] = M_KEY_SEMICOLON;
    keymap[VK_OEM_PLUS] = M_KEY_EQUAL;
    keymap[VK_OEM_COMMA] = M_KEY_COMMA;
    keymap[VK_OEM_MINUS] = M_KEY_MINUS;
    keymap[VK_OEM_PERIOD] = M_KEY_PERIOD;
    keymap[VK_OEM_2] = M_KEY_FORWARDSLASH;
    keymap[VK_OEM_3] = M_KEY_BACKTICK;
    keymap[VK_OEM_4] = M_KEY_LBRACKET;
    keymap[VK_OEM_5] = M_KEY_BACKSLASH;
    keymap[VK_OEM_6] = M_KEY_RBRACKET;
    keymap[VK_OEM_7] = M_KEY_APOSTROPHE;
}
