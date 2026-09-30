#include "../win.h"


//#include "../../platform/win32/platform.h"
#include "../../base/base_log.h"

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


#define WIN_CLASS_NAME "CustomWindow"

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


static int RegisterWinClass(void);

static LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    Window* win = (Window *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    /*
    PAINTSTRUCT ps;
    HDC hdc;

    const wchar_t* string = L"8KB application that uses GDI calls (With no uCRT)";
    */

    switch (umsg)
    {
        // Check if a key has been pressed on the keyboard.
        case WM_KEYDOWN:
        {
            DoKeyDown((unsigned int) wparam);
            return 0;
        } break;

        case WM_KEYUP:
        {
            // Input system.
            DoKeyUp((unsigned int) wparam);
            return 0;
        } break;

        // Check for mouse events.
        case WM_MOUSEMOVE:
        {
            input.mouseXPos = (float) ((lparam) & 0xffff);
            input.mouseYPos = (float) ((lparam >> 16) & 0xffff);
        } break;

        // Left, middle, right.
        case WM_LBUTTONDOWN: { input.mouseButtons[MOUSE_LEFT] = TRUE; } break;
        case WM_LBUTTONUP: { input.mouseButtons[MOUSE_LEFT] = FALSE; } break;
        case WM_MBUTTONDOWN: { input.mouseButtons[MOUSE_MIDDLE] = TRUE; } break;
        case WM_MBUTTONUP: { input.mouseButtons[MOUSE_MIDDLE] = FALSE; } break;
        case WM_RBUTTONDOWN: { input.mouseButtons[MOUSE_RIGHT] = TRUE; } break;
        case WM_RBUTTONUP: { input.mouseButtons[MOUSE_RIGHT] = FALSE; } break;

        case WM_MOUSEWHEEL:
        {
            // If it's not zoom gesture...
            //if () {
                float delta = (f32) GET_WHEEL_DELTA_WPARAM(wparam);
                input.mouseScrollY += delta / (float) WHEEL_DELTA;
            //}
        } break;
        /*
        case WM_PAINT:
        {
            hdc = BeginPaint(hwnd, &ps);
            // TODO: Add drawing code here...

            RECT rt;
            GetClientRect(hwnd, &rt);
            DrawTextW(hdc, string, WstrLen(string), &rt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hwnd, &ps);
        } break;
        */

        case WM_SIZE:
        {
            // TODO: Bruh.
            /*
            u32 w = (u32) LOWORD(lparam);
            u32 h = (u32) HIWORD(lparam);

            win->w = w;
            win->h = h;
            */
        } break;

        case WM_CLOSE:
        {
            win->flags |= WIN_FLAG_SHOULD_CLOSE;
        } break;

        default:
        {
            return DefWindowProcW(hwnd, umsg, wparam, lparam);
        } break;
    }

    return 0;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    switch (umessage)
    {
        // Cases from the ResterTek D3D11 tutorial.
        /*
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }
        */

        // All other messages pass through the message handler
        // function defined earlier.
        default:
        {
            return MessageHandler(hwnd, umessage, wparam, lparam);
        }
    }
}


extern Window* WindowInit(ARENA* arena, const wchar_t* title, int width, int height)
{
    //PlatformInit();
    LogInfo("Initializing window!");

    if (!windowInitialized)
    {
        windowInitialized = RegisterWinClass();
    }

    if (!windowInitialized)
    {
        // If it was still not initialized.
        // TODO: Do log call?
        LogErrorEmitF("%s: Failed to initialize window!\n", __func__);
        return NULL;
    }

    ARENA_TEMP arenaTemp = ArenaTempBegin(arena);


    Window* win = ArenaPushStruct(arenaTemp.arena, Window);
    win->title = title;
    win->w = width;
    win->h = height;
    win->flags = 0;
    win->backend = ArenaPushStruct(arenaTemp.arena, WinBackend);

    RECT winRect = { 0, 0, (int) width, (int) height };
    if (!AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, FALSE))
    {
        // TODO: Print call?
        LogErrorEmitF("%s: Failed to adjust window rect!\n", __func__);
        goto fail;
    }

    win->backend->hwnd = CreateWindowW(
        L"CustomWindow",
        win->title, WS_OVERLAPPEDWINDOW, // WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_OVERLAPPEDWINDOW,
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
    win->user = &win->backend->hwnd;

    SetWindowLongPtrW(win->backend->hwnd, GWLP_USERDATA, (LONG_PTR) win);

    // Init input.
    {
        _MEMSET(&input, 0, sizeof(input));

        input.mouseXPos = 0;
        input.mouseYPos = 0;

        // Initialize all the keys.
        for (int i = 0; i < 256; ++i)
        {
            input.keys[i] = FALSE;
        }
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
    ArenaTempEnd(arenaTemp);
    return NULL;


    // TODO: implement things.

    /*
    Window* win = ArenaPushStruct(w32Arena, Window);
    win->backend = ArenaPushStruct(w32Arena, WinBackend);

    win->backend->hwnd = CreateWindow(
        "Engine Window",
        "Engine Window",
        WS_OVERLAPPEDWINDOW,
        20, // Starting X.
        20, // Starting Y.
        640, 480,
        NULL, NULL, hinstance, NULL
    );

    if (!hwnd)
    {
        return NULL;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return window;
    */
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

    UnregisterClassW(win->title, win->backend->hinstance);
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
    MSG msg = { 0 };
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}



/*
   Input.
*/

extern void InputTerminate(void)
{
    input.mouseXPos = 0;
    input.mouseYPos = 0;
}


extern int DoKeyDown(unsigned int key)
{
    input.keys[key] = TRUE;
    return 0;
}
extern int DoKeyUp(unsigned int key)
{
    input.keys[key] = FALSE;
    return 0;
}



extern int IsKeyDown(unsigned int key)
{
    return input.keys[key];
}
extern int IsKeyUp(unsigned int key)
{
    return !input.keys[key];
}


extern int SetMouseXPosition(float x)
{
    input.mouseXPos = x;
    return 0;
}
extern int SetMouseYPosition(float y)
{
    input.mouseYPos = y;
    return 0;
}

extern float GetMouseXPosition(void)
{
    return input.mouseXPos;
}
extern float GetMouseYPosition(void)
{
    return input.mouseYPos;
}



/*
   Window.
*/

extern _inline int WindowOpened(Window* win)
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

    /*
    WNDCLASSEX wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = moduleHandle;
    wc.cbClsExtra = 0;
    wc.hIcon = LoadIcon(wc.hInstance, (LPCTSTR) IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WIN_CLASS_NAME;
    //wc.hIconSm = LoadIcon(wc.hInstance, (LPCTSTR) IDI_APPLICATION);
    */
    /*
    WNDCLASSW wc;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = moduleHandle;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = WIN_CLASS_NAME;

    ATOM atom = RegisterClassW(&wc);
    */
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
