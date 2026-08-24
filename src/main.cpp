#include "app.h"
#include "app.cpp"

//#include "graphics/gfx.h"
#include "graphics/draw3d.h"
#include "graphics/camera.cpp"
#include "graphics/draw3d.cpp"
#include "graphics/gfx.cpp"
#include "graphics/model.cpp"
#include "graphics/font.cpp"
#include "graphics/texture.cpp"

#include "ttf/truetype_renderer.cpp"
#include "ttf/truetype_parse.cpp"

// Memory allocation.
#include "misc/stack.cpp"
#include "misc/mem.cpp"
#include "misc/arena.cpp"

#include "asset.cpp"

#include "platform/input.h"
#include "platform/input.cpp"
#include "platform/window.h"

// Runtime.
#include "runtime/anm_manager.h"
#include "runtime/anm_manager.cpp"
#include "runtime/vm_assembly.h"
#include "runtime/vm_assembly.cpp"

//#include "base/base_log.h"


#include "base/mathf.cpp"

// TODO: GraphicsInit/GraphicsTerminate is also responsible
// for initializing/releasing the input system, so maybe we
// should separate it by concerns.

int Test_Mesh(void)
{
    MESH mesh;
    MeshLoad_OBJ(&mesh, "assets/utah-teapot.obj");

    return 0;
}



int Test_Game(void)
{
    // Get pointer to global window handle.
    if (DrawInit())
    {
        fprintf(stderr, "%s: Failed to initialize graphics subsystem!\n", __func__);
        return -1;
    }

    WINDOW* window = windowHandle;

    // Initialize application.
    if (AppInit(window))
    {
        fprintf(stderr, "%s: Failed to initialize application client!\n", __func__);
        return -1;
    }


    //fprintf(stderr, "Initialized application!\n"); 

    // Run graphics system.
    //GraphicsRun();
    //fprintf(stderr, "%s: Sus.\n", __func__);

    ANM_Init();

    ANM_VM* vm = ANM_CreateVM();

    float t{0};

    while (WindowOpened(window))
    {
        startedTime = PlatformTimeUsec();

        WindowProcessEvents(window);
        if (IsKeyDown(VK_ESCAPE))
        {
            fprintf(stderr, "%s: alright, escaping...\n", __func__);
            break;
        }

        // BEFORE: Update camera.
        // We calculate dt.
        double dt = deltaTimeU * 0.0001f;
        Camera3Step(&camera3Handle, (float) dt);

        //fprintf(stderr, "%s: DeltatimeU = %f\n", __func__, (float) deltaTimeU);


        DrawClear(0x0000FF);
        DrawReset();
        DrawSetTarget();

        ShaderUse(defaultShader);
        DrawTexture(0, defaultSquareTexture);

        DrawColor(0, 0, 0, 1.0);
        DrawColor2(0, 0, 0, 0);
        DrawColorMode(COLOR_LR);
        DrawRect(2, 2);

        //DrawColor(0, 0, 0, 0);
        //DrawColor2(0, 0, 0, 0.5);
        //DrawColorMode(COLOR_LR);
        //DrawRect(2, 2);

        /*
        DrawMatIdentity();
        DrawMatTranslate3D(0.3, 0.1, 0.5);
        DrawColor(1, 1, 1, 1);
        DrawColor2(1, 1, 0, 1);

        DrawColorMode(COLOR1);
        DrawRect(1, 1);
        */

        DrawMatIdentity();
        DrawMatTranslate3D(-0.4, +0.5, 0.0);
        DrawColorMode(COLOR2);
        DrawRect(0.25, 0.5);

        DrawMatIdentity();
        DrawMatTranslate3D(-0.9, -0.5, 0.0);
        DrawColorMode(COLOR_UD);
        DrawColor(1, 0, 0, 1);
        DrawColor2(1, 0, 0, 0);
        DrawRect(0.75, 0.1);

        // UI.
        for (int i = 0; i < 10; ++i)
        {
            DrawColor(1, 1, 1, 1);
            DrawColor2(0, 0, 0, 0);
            DrawMatIdentity();
            DrawColorMode(COLOR_INOUT);
            //DrawLine(-1.0f, -1.0f, 1.0f, 1.0f, (sin(t) + 1.0f) * 0.5f);
            DrawEllipse(5 + i, 0.8f - i * 0.1f, 0.8f - i * 0.1f);
        }

        DrawColor(0, 1, 0, 1);
        DrawColor2(0, 1, 0, 1);
        DrawMatIdentity();
        DrawMatTranslate(0.4f, 0.4f);
        DrawColorMode(COLOR_INOUT);
        //DrawArcSector(5, t, 5, 0.3, 0.05);

        //LINEPATH path;
        //LinePathPushStar(&path, 5, 0, 0.1, 0.1);

        //DrawLinePath(path);
        /*
        float points[8] = {
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 0.5f,
            -0.4f, 0.25f,
        };
        */

        float points[64];
        int p = 5;
        LinePathCreateStar(points, 64, p, t, 0.1f, 0.2f);

        DrawLinePathEx(points, p*2+1, 0.03f, LINE_PATH_CLOSED);

        DrawColor(1, 0, 0, 1);
        DrawColor2(1, 0, 0, 1);
        DrawColorMode(COLOR_INOUT);

        for (int i = 0; i < p*2; ++i)
        {
            DrawMatIdentity();
            DrawMatTranslate3D(points[i*2 + 0], points[i*2+1], 0);
            DrawRect(0.03f, 0.03f);
        }

        //VM_Draw(vm);

        // TODO: pro? Textures aren't being used if the draw command isn't made,
        // which doesn't register in RenderDoc.
        //DrawTextPro(defaultFont, "Hello", 0, 0, 40, 4.0, 0, 0.01, NULL);

        DrawFlush();

        // Present swapchain to the screen.
        DrawEnd();

        t += dt * 0.01;
        deltaTimeU = PlatformTimeUsec() - startedTime;
        //if (deltaTimeU < DTIME) {
        //    PlatformSleepMs(DTIME - deltaTimeU);
        //}
    }

    // Release the application.
    ANM_Terminate();

    AppShutdown();

    DrawTerminate();

    return 0;
}

/*
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, PSTR pScmdline, int iCmdshow)
{
    return Test_Game();
}
*/

int main(int argc, const char** argv)
{
    return Test_Game();
}
