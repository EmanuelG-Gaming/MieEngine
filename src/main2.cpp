#define UNICODE
#define _UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

//#include "../ext/arena.cpp"

#include "base/base_log.h"
#include "base/base_string.cpp"
#include "base/math/mathf.cpp"

#include "graphics/graphics.cpp"
#include "base/base_log.cpp"
#include "graphics/draw.cpp"
#include "platform/platform.cpp"

#include "../ext/arena.cpp"



int ExampleTesting(void)
{
    {
    //    LogFrameBegin();
    }

    //ARENA* arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);
    Window* window = WindowInit(L"Example window", 640, 480);
    GraphicsInit(window);

    {
    //   String8 res = LogFrameEnd(arena, LOG_ALL, LOG_RES_CONCAT, TRUE);
    //    FastPrint(res);
    }

    int t = 1;
    while (WindowOpened(window))
    {
        // Poll events.
        WindowProcessEvents(window);

        // RGB format: (0, 17, 85).
        DrawClear(0x001155);
        DrawReset();
        DrawSetTarget();

        DrawBegin();

        DrawColor(0, 0, 0, 1.0);
        DrawColor2(0, 0, 0, 0);
        DrawColorMode(COLOR_LR);

        DrawRect(0.1, 0.1);
        //DrawMatTranslate3D(0.1f, 0.5f, 0.0f);

        /*
        for (int i = 0; i < t; ++i)
        {
            DrawRect(0.5, 0.5);
        }

        //printf("t: %d\n", t);


        DrawRect(0.5, 0.5);
        */


        DrawFlush();

        //DrawRenderMesh(mesh);

        // Present the framebuffer to the screen.
        DrawEnd();

        t++;
    }

    GraphicsTerminate();
    WindowTerminate(window);

    PlatformTerminate();

    //ArenaTerminate(arena);




    return 0;

}

int main(void)
{
    ExampleTesting();
    return 0;
}
