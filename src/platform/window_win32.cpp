#include "window.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <windef.h>
#include <windows.h>
#include <winnt.h>
#include <winuser.h>

#include "platform.h"
#include "../base/base_defs.h"


/*
   DPI Scaling is a feature on Windows 8.1 and above,
   which is used to correctly display text.
*/
//#define USE_DPI 1

#if defined(USE_DPI)
#include <shellscalingapi.h>
#endif


#define WIN_CLASS_NAME L"CustomWindow"


typedef struct WINBACKEND {
    HINSTANCE hinstance;
    HWND hwnd; // Handle window.
} WINBACKEND;

WINDOW* windowHandle = NULL;
GRAPHICS graphics;

static b32 platformInitialized = FALSE;
static b32 windowInitialized = FALSE;
static u64 perfFreq = 1;

static int RegisterWinClass(void);

static LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    WINDOW* win = (WINDOW *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

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
            return DefWindowProc(hwnd, umsg, wparam, lparam);
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


static void UpdateWindowDPI(WINDOW* win)
{
    u32 rawDpi = 96;
    u32 effectiveDpi = 96;

#if defined(USE_DPI)
    HMONITOR monitor = MonitorFromWindow(win->backend->hwnd,
        MONITOR_DEFAULTTONEAREST);

    GetDpiForMonitor(monitor, MDT_RAW_DPI, &rawDpi, &rawDpi);
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &effectiveDpi, &effectiveDpi);
#endif

    win->dpi = effectiveDpi;
    win->rawDpi = rawDpi;
}

/*
   The window.
*/

extern int PlatformInit(void)
{
    if (platformInitialized == TRUE)
    {
        fprintf(stderr, "%s: Platform already initialized!\n", __func__);
        return -1;
    }

    LARGE_INTEGER pFreq = { 0 };
    if (QueryPerformanceFrequency(&pFreq))
    {
        perfFreq = (u64) pFreq.QuadPart;
    }
    else
    {
        fprintf(stderr, "%s: Failed to query performance frequency!\n", __func__);
        return -1;
    }

    if (InputInit())
    {
        fprintf(stderr, "%s: Failed to init input system!\n", __func__);
        return -1;
    }

    platformInitialized = TRUE;

    return 0;
}


extern WINDOW* WindowInit(const wchar_t* title, int width, int height)
{
    PlatformInit();

    if (!windowInitialized)
    {
        windowInitialized = RegisterWinClass();
    }

    if (!windowInitialized)
    {
        fprintf(stderr, "%s: window not initialized.\n", __func__);
        return NULL;
    }

    /*
    // This might not work if the window is encoded differently.
    _MEMSET(win, 0, sizeof(*win));
    win->title = (const wchar_t *) title;
    win->w = width;
    win->h = height;
    win->flags = 0;

    // Init backend.
    win->backend = (WINBACKEND *) _MALLOC(sizeof(WINBACKEND));
    if (!win->backend) {
        fprintf(stderr, "%s: Couldn't initialize window backend!\n", __func__);
        return -1;
    }
    _MEMSET(win->backend, 0, sizeof(WINBACKEND));
    */

    // We get the module handle (maybe we should do this once).
    //win->backend->hinstance = GetModuleHandle(NULL);


    //DEVMODE dmScreenSettings;


    // This might not work if the window is encoded differently.
    WINDOW* win = (WINDOW *) _MALLOC(sizeof(WINDOW));
    if (win == NULL)
    {
        fprintf(stderr, "%s: Failed to allocate window!\n", __func__);
        return NULL;
    }
    _MEMSET(win, 0, sizeof(*win));
    win->title = (const wchar_t *) title;
    win->w = width;
    win->h = height;
    win->flags = 0;



    // Init backend.
    win->backend = (WINBACKEND *) _MALLOC(sizeof(WINBACKEND));
    if (!win->backend)
    {
        fprintf(stderr, "%s: Couldn't initialize window backend!\n", __func__);
        return NULL;
    }
    _MEMSET(win->backend, 0, sizeof(WINBACKEND));


    RECT winRect = { 0, 0, (int) width, (int) height };
    if (!AdjustWindowRect(&winRect, WS_OVERLAPPEDWINDOW, FALSE))
    {
        fprintf(stderr, "%s: Failed to adkist window rect.\n", __func__);
        return NULL;
    }

    /*
    WNDCLASSEX wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = win->backend->hinstance;
    wc.hIcon = LoadIcon(wc.hInstance, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    //wc.lpszClassName = UNICODE_CAST(win->title);
    wc.lpszClassName = win->title;
    wc.cbSize = sizeof(WNDCLASSEX);

    if (!RegisterClassEx(&wc)) {
        fprintf(stderr, "%s: Failed to register window!\n", __func__);
        return -1;
    }
    */

    // Determine the resolution of the client's desktop screen.
    // WARN: Uhh...
    /*
    win->w = GetSystemMetrics(SM_CXSCREEN);
    win->h = GetSystemMetrics(SM_CYSCREEN);

    // Setup the screen settings based on whether or
    // not we're running in fullscreen or not.
    if (FULLSCREEN) {
        // Well we use pellets (actual pixels).
        _MEMSET(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = (unsigned long) win->w;
        dmScreenSettings.dmPelsHeight = (unsigned long) win->h;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        // Change display settings to fullscreen.
        ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

        // Set the position of the window to top-left corner.
        posx = posy = 0;
    } else {
        // If windowed, set to 800x600 resolution.
        //win->w = 800;
        //win->h = 600;
        win->w = width;
        win->h = height;

        // Place the window in the middle of the screen.
        posx = (GetSystemMetrics(SM_CXSCREEN) - win->w) / 2;
        posy = (GetSystemMetrics(SM_CYSCREEN) - win->h) / 2;
    }
    */


    // NOTE: WS_OVERLAPPEDWINDOW is for windows that are affected by the WM.
    win->backend->hwnd = CreateWindowW(
        /*WS_EX_APPWINDOW, */WIN_CLASS_NAME,
        win->title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winRect.right - winRect.left, winRect.bottom - winRect.top,
        NULL, NULL, NULL, NULL
    );
    if (win->backend->hwnd == NULL)
    {
        fprintf(stderr, "%s: Failed to create window!\n", __func__);
        return NULL;
    }


    // Set window data for input.
    // TODO: This doesn't work because Windows immediately sends WM_SIZE messages.

    SetWindowLongPtrW(win->backend->hwnd, GWLP_USERDATA, (LONG_PTR) win);

    UpdateWindowDPI(win);



    // Bring the window up to the screen and set it as main focus.
    ShowWindow(win->backend->hwnd, SW_SHOW);
    //UpdateWindow(win->backend->hwnd);
    //SetForegroundWindow(win->backend->hwnd);
    //SetFocus(win->backend->hwnd);

    // Hide the mouse cursor.
    //ShowCursor(FALSE);

    return win;
}


extern void WindowTerminate(WINDOW* win)
{
    if (win == NULL)
    {
        return;
    }

    // Show the mouse cursor.
    ShowCursor(TRUE);

    if (FULLSCREEN)
    {
        ChangeDisplaySettings(NULL, 0);
    }

    // Remove the window.
    DestroyWindow(win->backend->hwnd);
    win->backend->hwnd = NULL;

    // Remove the application's instance.
    //UnregisterClass(UNICODE_CAST(win->title), win->backend->hinstance);
    UnregisterClass(win->title, win->backend->hinstance);
    win->backend->hinstance = NULL;

    // Release the input.
    InputTerminate();

    // Remove backend.
    if (win->backend)
    {
        _FREE(win->backend);
        win->backend = NULL;
    }

    _FREE(win);
}


extern void WindowProcessEvents(WINDOW* win)
{
    // Fill input data.
    // Processing events.
    MSG msg = { 0 };
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

extern int WindowOpened(WINDOW *win)
{
    return (win->flags & WIN_FLAG_SHOULD_CLOSE) == 0;
}

static int RegisterWinClass(void)
{
    HINSTANCE moduleHandle = GetModuleHandleW(NULL);
    if (moduleHandle == NULL)
    {
        fprintf(stderr, "%s: Failed to get module handle.\n", __func__);
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
        fprintf(stderr, "%s: Failed to register window class.\n", __func__);
        return FALSE;
    }

    return TRUE;
}


/*
   Platform-specific things.
*/

extern void PlatformSleepMs(unsigned int ms)
{
    Sleep(ms);
}
extern u64 PlatformTimeUsec(void)
{
    LARGE_INTEGER ticks = { 0 };

    if (!QueryPerformanceCounter(&ticks))
    {
        fprintf(stderr, "%s: Failed to query performance counter!\n", __func__);
        return 0;
    }

    return (u64) ticks.QuadPart * 1000000 / perfFreq;
}
