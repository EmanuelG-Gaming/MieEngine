/*
   Refer to the Advanced 3D game programming textbook with DirectX9.
*/

// TLDR: always make sure that you define
// the Windows compile-time config macros.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE



//#include "win/win.cpp"



#include "../ext/arena.cpp"

#include "graphics/graphics.cpp"

#include "mem/arena.h"


#include "audio/audio.h"
#include "base/base_defs.h"

#include "graphics/mesh.h"
//#include "mem/arena.h"
//#include "../ext/arena.cpp"
#include "graphics/texture.h"


#include "platform/platform.h"


// Logging/strings.
#include "base/base_string.h"
#include "base/base_string.cpp"
#include "base/base_log.h"
#include "base/base_log.cpp"
#include "base/base_fmt.h"
#include "base/base_fmt.cpp"

// Math.
#include "base/math/mathf.h"
#include "base/math/mathf.cpp"
#include "base/math/base_rng.h"
#include "base/math/base_rng.cpp"

// Graphics.
#include "graphics/draw.h"
#include "graphics/draw.cpp"


#include "graphics/mesh.h"
#include "graphics/mesh.cpp"

#include "graphics/camera.h"
#include "graphics/camera.cpp"


#include "graphics/font/font.h"
#include "graphics/font/font.cpp"
#include "graphics/image/image.h"
#include "graphics/image/image.cpp"
#include "graphics/texture.h"
#include "graphics/texture.cpp"



// Assets
#include "../ext/lzss.cpp"
#include "io/asset.h"
#include "io/asset.cpp"


// Audio.
#include "audio/audio.h"
#include "audio/audio.cpp"


// ANM.
#include "core/timer.h"
#include "core/timer.cpp"
#include "core/interp.h"
#include "core/interp.cpp"

#include "runtime/anm_vm.h"
#include "runtime/anm_vars.h"
#include "runtime/anm_vm.cpp"

#include "runtime/anm_manager.h"
#include "runtime/anm_manager.cpp"

//#include "mem/arena.cpp"

#include "win/win.h"
//#include "win/win.cpp"

#include "platform/platform.cpp"

AnmManager* g_anmManager = NULL;

static inline float EasingNone(float t)
{
    return 1.0f;
}
static inline float EasingLinear(float t)
{
    return t;
}
static inline float EasingQuadratic(float t)
{
    return t*t;
}
static inline float EasingCubic(float t)
{
    return t*t*t;
}

static inline float EasingQuadratic_inOut(float t)
{
    if (t < 0.5f)
    {
        return 2.0f*t*t;
    }
    else
    {
        return -1.0f + (4.0f - 2.0f * t) * t;
    }
}

static inline float EasingQuadratic_spike(float t)
{
    float off = t - 0.5f;
    return -off*off*4 + 1;
}
static inline float EasingQuartic_spike(float t)
{
    float off = t - 0.5f;
    static const float twoPow4 = 16;
    return -off*off*off*off*twoPow4 + 1;
}


typedef struct FlashingImages {
    GFX_texture** textures;
    BlendMode* blends;

    float timer;
    float interpTimers[16];
    int imageCap;

    // Some linear interpolation can be used.
    float (*easingFunc)(float);

    float nextDuration;
    b32 active;

    int beginIndex;
    int presImageCount;
} FlashingImages;

static int FlashingImagesInit(FlashingImages* res, GFX_texture** textures, BlendMode* blends, int nTex)
{
    res->imageCap = nTex;
    res->textures = textures;
    res->blends = blends;

    res->timer = 0.0f;
    _MEMSET(res->interpTimers, 0, sizeof(res->interpTimers));

    res->easingFunc = EasingQuadratic_spike;

    res->nextDuration = 0.5f;
    res->active = TRUE;

    res->beginIndex = 0;
    res->presImageCount = 1;

    return 0;
}




static DrawPass passes[8];
static mat4 viewMat2D = mat4 { 1.0f };
static mat4 viewMat3D = mat4 { 1.0f };

static CAMERA3D camera3Handle { 0.0f };

static FlashingImages flashingImages = { 0 };


// TODO: Use framebuffer texture.
void GetBillboardRotMatrix(float* mat, CAMERA3D const* camera, vec3 billboardPos)
{
    vec3 dir = (camera->pos - billboardPos).Nor();
    vec3 right = (vec3::Cross(dir, camera->up)).Nor();
    vec3 up = vec3::Cross(right, dir);

    // Rotation matrix.
    //float*
    mat[0] = right.x;
    mat[4] = right.y;
    mat[8] = right.z;
    mat[12] = 0.0f;

    mat[1] = up.x;
    mat[5] = up.y;
    mat[9] = up.z;
    mat[13] = 0.0f;

    mat[2] = dir.x;
    mat[6] = dir.y;
    mat[10] = dir.z;
    mat[14] = 0.0f;

    mat[3] = 0.0f;
    mat[7] = 0.0f;
    mat[11] = 0.0f;
    mat[15] = 1.0f;
}

GFX_texture* Load2x2PixelSquare(ARENA* arena)
{
    u32 pix[4] = {
        0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff,
    };

    return LoadImmutableTextureFromPixels(arena, 2, 2, reinterpret_cast<unsigned char *>(pix), 0);
}


unsigned char DOOM_firePallette[37*4] = {
    7, 7, 7, 7, // 255 - full opacity.
    31, 7, 7, 31,
    47, 15, 7, 47,
    71, 15, 7, 71,
    87, 23, 7, 87,
    103, 31, 7, 150,
    119, 39, 7, 150,
    143, 39, 7, 150,
    159, 47, 7, 160,
    175, 63, 7, 175,
    191, 71, 7, 191,
    199, 71, 7, 200,
    223, 79, 7, 223,
    223, 87, 7, 223,
    223, 87, 7, 223,
    215, 95, 7, 240,
    215, 95, 7, 255,
    215, 103, 15, 255,
    207, 111, 15, 255,
    207, 119, 15, 255,
    207, 127, 15, 255,
    207, 135, 23, 255,
    199, 135, 23, 255,
    199, 143, 23, 255,
    191, 151, 31, 255,
    191, 159, 31, 255,
    191, 167, 39, 255,
    191, 167, 39, 255,
    191, 175, 47, 255,
    183, 175, 47, 255,
    183, 183, 47, 255,
    183, 183, 55, 255,
    207, 207, 111, 255,
    239, 239, 100, 255,
    223, 223, 159, 255,
    239, 239, 199, 255,
    255, 255, 255, 255,
};

unsigned char sinisterPallete[] = {
    200, 0, 0, 255,
    200, 60, 60, 255,
    255, 0, 0, 255,
    255, 140, 140, 255,
};

#define PAL_SIZE(arr) (sizeof(arr) / 4)

void SpreadFire(int* buf, int w, int h, int src)
{
    int len = w*h;

    int pixel = buf[src];
    if (pixel == 0)
    {
        if (src >= w)
        {
            buf[src+w] = 0;
        }
    }
    else
    {
        int randIndex = static_cast<int> (RNG_randf32_range(0, 3.0f));
        int dst = src + randIndex - 1;

        // Fire spreads from the line at the first row of the memory framebuffer.
        // Bounds checking.
        if (dst >= 0 && dst < len)
        {
            int newPixel = pixel - (randIndex & 1);
            if (newPixel >= 0)
            {
                int dstIdx = dst + w;
                if (dstIdx >= 0 && dstIdx < len)
                {
                    buf[dstIdx] = newPixel;
                }
            }
        }
    }
}

int WindowTesting(void)
{
    PlatformInit();

    // Begin logging things.
    LogFrameBegin();

    // Initialize first.
    AssetArchive(0, "assets.bin");


    // More bytes neeeded.
    ARENA* arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);
    Window* window = WindowInit(L"Testing window", 640*1.5, 480*1.5);

    // Create graphics.
    GraphicsInit(window);


    //AudioInit(window);

    // TODO: Fix for arena.

    //u8* arr = ArenaPushArrayZero(arena, u8, 6700);

    // TODO: Audio playback issue (multithreading is maybe the solution).

    //AUD_sound* sound = AudioLoadSound_wav(arena, "assets/csfteow.wav");
    //AudioSoundPlay(sound);


    GFX_texture* pixSquare = Load2x2PixelSquare(arena);

    GFX_font* font = FontInit(arena);
    FontAtlasPage* page = FontPageInit(font, 0);
    FontLoadBitmapDefault(font, 0);
    FontPageAdd_ascii(font, 0);

    GFX_texture* fontTex = FontUploadTexture(font, 0);


    GFX_font* font2 = FontInit(arena);
    FontAtlasPage* page2 = FontPageInit(font2, 0);
    //FontLoadTTF(font2, 0, "assets/fonts/Exo-Bold.ttf");
    FontLoadTTF(font2, 0, "assets/fonts/Charakterny.ttf");


    GFX_texture* fontTex2 = FontUploadTexture(font2, 0);




    /*
    CustomVertex vertices[] = {
        // Color is in ARGB format.
        { 0.0f, 0.0f, 0.5f, 0, 0xffff0000, 1.0f, 0.5f },
        { 0.5f, 1.0f, 0.5f, 0, 0xff00ff00, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.5f, 0, 0xff0000ff, 0.0f, 0.0f },
    };
    */



    //GFX_mesh* mesh = DrawUploadMesh(arena, vertices, 3);
    MeshBuilder builder = MeshCreateCuboid(arena, 1, 1, 1);
    GFX_mesh* mesh = MeshBuilderCreateMesh(&builder, arena);

    GFX_texture* texture = LoadTexture(arena, "images/code.png");

    //GFX_texture* textureCube = LoadTexture(arena, "images\\lesanae.jpeg");
    GFX_texture* zoeTexture = LoadTexture(arena, "images/zoe.png", TEXTURE_ALPHA);
    GFX_texture* programTexture = LoadTexture(arena, "images/programming.png");
    GFX_texture* perlTexture = LoadTexture(arena, "images/perl-be-like.png");
    GFX_texture* smallExecTexture = LoadTexture(arena, "images/small.png");
    GFX_texture* flareTexture = LoadTexture(arena, "images/spark1.png", TEXTURE_ALPHA);


    // Generate some glitch image.
    SoftImage* glitch = SoftImageInitOrigin(arena, 512, 512, 4);
    SoftImage_generateGlitchBIOS(glitch, 0.5f);
    GFX_texture* glitchTex = LoadImmutableTextureFromPixels(arena, glitch->width, glitch->height, glitch->data, 0);

    // DOOM fire viewport texture.
    GFX_texture* fireTex = AllocateMutableTexture(640, 480, NULL, 0);
    GFX_texture* sinisterTex = AllocateMutableTexture(320, 240, NULL, 0);

    int* fireBuffer = new int[fireTex->w*fireTex->h];
    _MEMSET(fireBuffer, 0, sizeof(int) * fireTex->w*fireTex->h);

    for (int x = 0; x < fireTex->w; ++x)
    {
        fireBuffer[x] = 36;
    }

    //GFX_shader* distortionShader = ShaderLoad(SHADER_LAYOUT_BASIC_SHADED, "assets/shaders/base.vso", "assets/shaders/distort.pso");

    // Load flashing images.
    // Might use some procedurally-generated glitch textures.

    // Sprites.
    GFX_texture* textures[] = {
        programTexture, fontTex, texture, fontTex2, perlTexture, smallExecTexture, glitchTex,
    };
    BlendMode blends[] = {
        BLEND_ADD, BLEND_ADD, BLEND_ADD, BLEND_ADD, BLEND_ADD, BLEND_ADD, BLEND_ADD,
    };

    FlashingImagesInit(&flashingImages, textures, blends, STATIC_ARR_LEN(textures));
    flashingImages.presImageCount = 2;
    flashingImages.active = FALSE;



    vec4 eye {0,0,-5,1};
    vec4 center { 0,0,1,1};
    vec4 up{0,1,0,1};

    Camera3Init(&camera3Handle);

    Mat4LookAt_LH(&viewMat3D, &eye, &center, &up);

    // And then load our passes.
    DrawSetPipeline(3, passes);

    // 1st pass.
    DrawSetPass2D(&passes[0]);
    passes[0].active = TRUE;
    passes[0].viewMatrix = &viewMat2D;
    //passes[0].flags |= DRAW_PASS_FLAG_GAMMA;
    DrawSetTarget();

    // 2nd pass.
    DrawSetPass3D(&passes[1]);
    passes[1].active = TRUE;
    passes[1].viewMatrix = &viewMat3D;
    //passes[0].flags |= DRAW_PASS_FLAG_GAMMA;
    DrawSetTarget();

    // 3rd pass.
    DrawSetPass3D(&passes[2]);
    passes[2].active = TRUE;
    passes[2].viewMatrix = &viewMat2D;
    //passes[0].flags |= DRAW_PASS_FLAG_GAMMA;
    DrawSetTarget();


    /*
       ANM logic.
    */

    //g_anmManager = (AnmManager *) malloc(sizeof(AnmManager));
    //AnmVM* allocVm = AnmManager::allocateVm();

    AnmVM_rawInstr instr[] = {
        // If the bit at the varMask is 1, then it's a literal.
        // Opcode, offset, time, varMask (MSB<-LSB), args.

        // x,y,z args (quaternion axis-angle rotation)
        //{ ANM_ANGLE_VEL, ANM_NEXT, 0, 0b1111, { F32LIT(0), F32LIT(0), F32LIT(0.01f) }},
        { ANM_SCALE, ANM_NEXT, 0, 0b1111, { F32LIT(0.05f), F32LIT(0.05f) } },
        { ANM_BLENDMODE, ANM_NEXT, 0, 0b1111, { BLEND_ADD }},

        { ANM_DESTROY, 0, 0, 0b0000, { 0 } },
    };

    AnmVM_rawInstr parentInstr[] = {
        //{ ANM_ROTATE, ANM_NEXT, 0, 0b1111, { F32LIT(0), F32LIT(0), F32LIT(-0.6f) }},
        //{ ANM_POS, ANM_NEXT, 0, 0b1111, { F32LIT(0.0f), F32LIT(-0.5f), F32LIT(0.0f) }},
        //{ ANM_SCALE_GROWTH, ANM_NEXT, 0, 0b11111, { F32LIT(0.01f), F32LIT(0.01f) }},
        { ANM_ANGLE_VEL, ANM_NEXT, 0, 0b1111, { F32LIT(0), F32LIT(0), F32LIT(0.001f) }},

        { ANM_DESTROY, 0, 0, 0b0000, { 0 } },
    };

    AnmVM vm{};
    vm.init(&vm);
    vm.loadScript(&vm, instr);

    AnmVM parentvm{};
    parentvm.init(&parentvm);
    parentvm.loadScript(&parentvm, parentInstr);


    //vm.pendingInterrupt = 0;


    // Post the logs to a buffer that gets sent to WriteFile Windows syscall,
    // using FastPrint() function for platform-dependent stuff.
    {
        String8 res = LogFrameEnd(arena, LOG_ALL, LOG_RES_CONCAT, TRUE);
        FastPrint(res);
    }

    u64 startedTime = 0;
    u64 deltaTimeU = 0;
    float t = 0.0f;

    while (WindowOpened(window))
    {
        startedTime = PlatformTimeUsec();

        // BEFORE: Update camera.
        // We calculate dt.
        double dt = deltaTimeU * 0.0001f;
        Camera3Step(&camera3Handle, (float) dt);


        // Draw something.
        WindowProcessEvents(window);


        // Handle the logic of the images.
        if (flashingImages.active)
        {
            // Circular buffer here.
            if (flashingImages.timer >= flashingImages.nextDuration)
            {
                // March over to next object.
                flashingImages.beginIndex += 1;
                flashingImages.timer -= flashingImages.nextDuration;

            }


            for (int i = flashingImages.beginIndex; i < flashingImages.beginIndex + flashingImages.presImageCount; ++i)
            {
                // Loop over index.
                int idx = i % flashingImages.imageCap;

                flashingImages.interpTimers[idx] += dt*0.01f;
                if (flashingImages.interpTimers[idx] >= flashingImages.nextDuration)
                {
                    flashingImages.interpTimers[idx] -= flashingImages.nextDuration;
                }
            }

            flashingImages.timer += dt*0.01f;
        }

        // RGB format.
        DrawClear(0x001155);
        //DrawClear(0xffff00);

        DrawReset();
        DrawSetTarget();

        DrawBegin();
        ShaderUse(defaultShader);
        DrawTexture(0, pixSquare);


        if (1) {
            // 1st pass.
            drawState.currentPass = 0;
            //drawState.passes[drawState.currentPass].target = 0;

            DrawReset();
            DrawSetTarget();

            //DrawBlend(BLEND_ALPHA);

            DrawColor(0.01, 0.01, 0.05, 1);
            DrawColor2(0.01, 0.01, 0.05, 1);
            DrawColorMode(COLOR_LR);

            DrawTexture(0, pixSquare);
            DrawRect(2, 2);

            DrawColor(1, 1, 1, 1);
            DrawColor2(1, 1, 1, 1);
            DrawColorMode(COLOR_LR);

            DrawTexture(0, texture);
            DrawSkybox();


            // Draw sectors.
            if (0) {
                DrawTexture(0, glitchTex);
                DrawBlend(BLEND_ADD);

                DrawColor(1, 1, 1, 0.05f);
                DrawColor2(1, 1, 1, 0.05f);
                DrawColorMode(COLOR_INOUT);

                for (int i = 0; i < 20; ++i)
                {
                    DrawArcSector(8 + (i/2), i*0.1f, PI2, 0.2 + i * 0.25, 0.07f);
                    DrawArcSector(8 + (i/2), -i*0.1f, PI2, 0.2 + i * 0.25, 0.07f);
                }

                DrawBlend(BLEND_ALPHA);
            }

            // Draw glitch texture.
            if (0)
            {
                DrawBlend(BLEND_ADD);
                DrawColor(1, 1, 1, 1);
                DrawColor2(1, 1, 1, 1);
                DrawColorMode(COLOR_LR);

                DrawTexture(0, glitchTex);
                DrawMatIdentity();
                DrawRect(2, 2);

                DrawBlend(BLEND_ALPHA);
            }


            if (0) {
                DrawColor(1, 1, 1, 1);
                DrawColor2(1, 1, 1, 1);
                DrawColorMode(COLOR_LR);

                DrawTexture(0, fontTex2);
                DrawMatTranslate(0.5, 0);
                DrawRect(0.75, 1.5);

                DrawTexture(0, fontTex);
                DrawMatTranslate(0.0, -0.5);
                DrawRect(0.75, 1.5);

                DrawColor(1, 1, 1, 0.01);
                DrawColor2(1, 1, 1, 0.01);
                DrawColorMode(COLOR_LR);
            }

            //DrawBlend(BLEND_ADD);

            //DrawMatIdentity();
            //DrawTexture(0, textureCube);
            //DrawRect(2, 2);
            //DrawBlend(BLEND_ALPHA);

            if (1) {
                unsigned int rowPitch;
                unsigned char* buf = TextureLock(sinisterTex, &rowPitch, NULL);
                for (int y = 0; y < sinisterTex->h; ++y)
                {
                    int palIndex = RNG_boundedRand(PAL_SIZE(sinisterPallete));
                    unsigned char* palColor = sinisterPallete + (palIndex * 4);

                    int bufRowAddr = y * rowPitch;
                    for (int x = 0; x < sinisterTex->w; ++x)
                    {
                        unsigned char* pix = buf + (bufRowAddr + (x * 4));

                        float rand = RNG_randf32_range(1, 64);
                        float alpha = ((y*(((int) (rand)) >> 4))) % ((int) rand) / rand;

                        pix[0] = palColor[0] * alpha;
                        pix[1] = palColor[1] * alpha;
                        pix[2] = palColor[2] * alpha;
                        pix[3] = palColor[3] * alpha;
                    }
                }

                TextureUnlock(sinisterTex);
            }

            DrawColor(1, 1, 1, 1);
            DrawColor2(1, 1, 1, 1);
            DrawColorMode(COLOR_LR);

            // Draw stars.
            if (0) {
                // Run VM.
                DrawTexture(0, flareTexture);

                {
                    // Update parent VM first.
                    parentvm.run(&parentvm);
                    parentvm.update(&parentvm);
                }

                AnmVM::run(&vm);
                AnmVM::update(&vm);

                float vmx = vm.entityPos.x;
                float vmy = vm.entityPos.y;

                //RNG_seed(0, 2);
                for (int i = 0; i < 10000; ++i)
                {
                    float randAngle = RNG_randf32_range(0, PI2);
                    float randLength = RNG_randf32_range(0.01f, 2.0f);
                    float c = cos(randAngle), s = sin(randAngle);

                    vm.entityPos.x = c * randLength * randLength;
                    vm.entityPos.y = s * randLength * randLength;

                    AnmVM::draw(&vm, &parentvm);
                }

                vm.entityPos.x = vmx;
                vm.entityPos.y = vmy;
            }


            if (1) {
                for (int y = 0; y < fireTex->h; ++y)
                {
                    int rowAddr = fireTex->w * y;
                    for (int x = 0; x < fireTex->w; ++x)
                    {
                        SpreadFire(fireBuffer, fireTex->w, fireTex->h, rowAddr + x);
                    }
                }

                // Copy CPU data to GPU.
                unsigned int rowPitch;
                unsigned char* buf = TextureLock(fireTex, &rowPitch, NULL);
                for (int y = 0; y < fireTex->h; ++y)
                {
                    int bufRowAddr = y * rowPitch;
                    for (int x = 0; x < fireTex->w; ++x)
                    {
                        int fire = fireBuffer[(y * fireTex->w + x)];

                        unsigned char* pix = buf + (bufRowAddr + (x * 4));

                        _MEMCPY(pix, DOOM_firePallette + (fire * 4), 4);
                    }
                }

                // Blit texture.
                /*
                for (int y = 0; y < fireTex->h; ++y)
                {
                    _MEMCPY(buf + ((y * rowPitch)), src + ((y * rowPitch)), rowPitch);
                }
                */
                TextureUnlock(fireTex);
            }


            DrawTexture(1, sinisterTex);
            DrawTexture(0, fireTex);

            //ShaderUse(distortionShader);

            DrawMatIdentity();
            DrawMatRotate3D(0, 0, -PI);

            DrawRect(2, 2);
            DrawMatIdentity();

            ShaderUse(defaultShader);

            // Draw DOOM texture.
            if (0)
            {
                DrawBlend(BLEND_ADD);
                DrawTexture(0, fireTex);
                DrawMatIdentity();
                DrawRect(2, -2);
                DrawBlend(BLEND_ALPHA);
            }

            DrawTexture(0, pixSquare);

            // Draw from 1st font.
            /*
            if (0) {
                const char* text = "Holy fucking shit?!?1\n";
                float scale = 0.005f;
                float width = 0.0f, height = 0.0f;
                DrawTextGetSize(font, text, 1, 0, 0, scale, &width, &height);
                DrawTextPro(font, text, 0.3f-width*0.5f, 0.0f-height*0.5f, 1, 0.0f, 0.0f, scale, 0, NULL);

                text = "IS THAT A MOTHERFUCKING BITMAP FONT REFERENCE????\n";
                scale = 0.0035f;
                DrawTextGetSize(font, text, 1, 0, 0, scale, &width, &height);
                DrawTextPro(font, text, 0.3f-width*0.5f, -0.1f-height*0.5f, 1, 0.0f, 0.0f, scale, 0, NULL);

                text = "BITMAP FONTS ARE THE BEST FUCKING RENDERING!!!!11 NETHACK SO BADASSS!!111\n";
                scale = 0.0021f;
                DrawTextGetSize(font, text, 1, 0, 0, scale, &width, &height);
                DrawTextPro(font, text, 0.3f-width*0.5f, -0.2f-height*0.5f, 1, 0.0f, 0.0f, scale, 0, NULL);

                text = "ORAORAORAORAORAORAORAORA\nMUDAMUDAMUDMAMUDAMUDAMUDAMUDA\nWRYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n";
                scale = 0.0021f;
                DrawTextGetSize(font, text, 1, 0, 0, scale, &width, &height);
                DrawTextPro(font, text, 0.3f-width*0.5f, -0.3f-height*0.5f, 1, 0.0f, 0.0f, scale, 0, NULL);
            }
            */

            // Draw from 2nd font.
            if (0) {
                const char* text = "Some spinning VMs";
                float scale = 0.0035f;

                DrawColor(0.0, 0.0, 1, 1);
                DrawColor2(0.0, 0.0, 1, 1);
                DrawColorMode(COLOR_UD);
                DrawBlend(BLEND_ALPHA);

                DrawTexture(0, font2->atlas.pages[0].texture);
                DrawMatIdentity();
                DrawMatTranslate(-0.1f, 0.0f);
                DrawTextPro(font2, text, 1, 0.0f, 0.0f, scale, 0, NULL);

                DrawMatIdentity();
                /*
                DrawColorMode(COLOR_LR);
                DrawBlend(BLEND_ADD);
                DrawTextPro(font2, text, -0.1, 0, 1, 0.0f, 0.0f, scale, 0, NULL);

                DrawBlend(BLEND_ALPHA);
                */
            }

            if (1) {
                const char* text = "There is too much fire.";
                float scale = 0.004f;

                DrawColor(1, 1, 1, 0.8);
                DrawColor2(1, 1, 1, 0.8);
                DrawColorMode(COLOR_UD);
                DrawBlend(BLEND_ADD);

                mat4 mat { 1.0f };
                mat.v[0] = vec4 { 0.4f, -0.4f, 0.0f, 0.0f };
                mat.v[1] = vec4 { 0.0f, 1.0f, 0.0f, 0.0f };

                DrawSetMatrix(mat.c);
                //DrawMatTranslate(-0.5f, 0.5);
                DrawTexture(0, font2->atlas.pages[0].texture);
                DrawTextPro(font2, text, 1, 0.0f, 0.0f, scale, 0, NULL);
                DrawBlend(BLEND_ALPHA);
                DrawMatIdentity();
            }


            DrawFlush();
        }

        if (1) {
            // 2nd pass.
            drawState.currentPass = 1;

            DrawReset();
            DrawSetTarget();

            DrawColor(1, 1, 1, 1);
            DrawColor2(1, 1, 1, 1);
            DrawColorMode(COLOR_LR);

            //DrawTexture(0, textureCube);
            DrawTexture(0, pixSquare);

            // Set up camera.
            DrawMatIdentity();
            //DrawMatTranslate(1, 1);
                //DrawMatTranslate3D(sin(t)*1, 0, 5);
            //DrawMatTranslate3D(0, 0, 5);

            // Process view matrix.
            {
                viewMat3D = mat4::Look({ camera3Handle.pos.x, camera3Handle.pos.y, camera3Handle.pos.z, 0 }, { camera3Handle.front.x, camera3Handle.front.y, camera3Handle.front.z, 0 },  { camera3Handle.up.x, camera3Handle.up.y, camera3Handle.up.z, 0 });
            }

            //DrawSkybox();
            DrawColor(1, 1, 1, 1);
            DrawColor2(0, 0, 0, 0);
            DrawColorMode(COLOR_UD);

            // Draw her with different matrices.
            if (0)
            {
                for (int x = 0; x < 50; ++x)
                {
                    for (int y = 0; y < 50; ++y)
                    {
                        DrawMatIdentity();
                        DrawMatTranslate3D(x, 0, y);

                        //DrawMatRotateYA(t);

                        DrawRenderMesh(mesh);
                    }
                }
            }
            DrawColor(1, 1, 1, 1);
            DrawColor2(1, 1, 1, 1);


            // Draw effects.
            if (0) {
                //float mat[16];
                DrawMatIdentity();
                //GetBillboardMatrix(mat, &camera3Handle, { 10, 5, 10 });
                float mat[16];
                GetBillboardRotMatrix(mat, &camera3Handle, { 10, 5, 10 });

                DrawSetMatrix(mat);
                //DrawMatTranslate3D(10, 5, 10);

                // Draw some ellipses.
                DrawColorMode(COLOR_UD);
                DrawColor(1, 0, 0, 1);
                DrawColor2(1, 0, 0, 0);
                DrawRect(0.75, 0.1);

                for (int i = 0; i < 10; ++i)
                {
                    DrawColor(1, 1, 1, 1);
                    DrawColor2(0, 0, 0, 0);
                    DrawColorMode(COLOR_INOUT);
                    //DrawLine(-1.0f, -1.0f, 1.0f, 1.0f, (sin(t) + 1.0f) * 0.5f);
                    DrawEllipse(5 + i, 0.8f - i * 0.1f, 0.8f - i * 0.1f);
                }
            }

            // Draw text.
            if (0) {
                DrawTexture(0, DrawGetFboTexture(FBO_TEXTURE1_COLOR));
                DrawMatIdentity();
                DrawMatTranslate3D(10, 10, 10);
                DrawRectBillboard(1, 1);
            }

            DrawTexture(0, zoeTexture);

            // Draw Zoe.
            // Blue texture.
            DrawBlend(BLEND_ADD);
            DrawColor(0.3, 0.0, 1, 1);
            DrawColor2(0, 0, 0, 0);
            DrawColorMode(COLOR_UD);

            DrawMatIdentity();
            DrawMatTranslate3D(10, 5, 10.01);

            DrawRectBillboard(1.02, 1.02);

            // Red texture
            DrawBlend(BLEND_ADD);
            DrawColor(0.9, 0.0, 0.2, 1);
            DrawColor2(0, 0, 0, 0);
            DrawColorMode(COLOR_UD);

            DrawMatIdentity();
            DrawMatTranslate3D(10, 5, 10.02);

            DrawRectBillboard(1, 1);

            // Green texture
            DrawBlend(BLEND_ADD);
            DrawColor(0.2, 1, 0.2, 1);
            DrawColor2(0, 0, 0, 0);
            DrawColorMode(COLOR_UD);

            DrawMatIdentity();
            DrawMatTranslate3D(10, 5, 10.03);

            DrawRectBillboard(1.04, 1.04);

            // Draw base character.
            DrawBlend(BLEND_ALPHA);
            DrawColor(1, 1, 1, 0.9);
            DrawColor2(1, 1, 1, 0.9);
            DrawMatIdentity();
            DrawMatTranslate3D(10, 4.99, 10);

            DrawRectBillboard(0.95, 0.95);


            DrawFlush();
        }

        {
            drawState.currentPass = 0;

            DrawReset();
            DrawSetTarget();

            if (flashingImages.active)
            {
                DrawMatIdentity();

                // Handle drawing the flashing images.
                for (int i = flashingImages.beginIndex; i < flashingImages.beginIndex + flashingImages.presImageCount; ++i)
                {
                    // Loop over index.
                    int idx = i % flashingImages.imageCap;

                    GFX_texture* tex = flashingImages.textures[idx];
                    BlendMode blend = flashingImages.blends[idx];

                    float alpha = flashingImages.easingFunc(flashingImages.interpTimers[idx]/flashingImages.nextDuration);

                    float aspect = windowHandle->h / (float) windowHandle->w;
                    //float aspect = 1.0f;

                    //float width = ((tex->w / (float)windowHandle->w) / aspect)*2.0f;
                    //float height = ((tex->h / (float) windowHandle->h) / aspect)*2.0f;
                    float width = 2.0f;
                    float height = 2.0f;

                    DrawBlend(blend);
                    DrawTexture(0, tex);

                    DrawColor(1, 1, 1, alpha);
                    DrawColor2(1, 1, 1, alpha);
                    DrawColorMode(COLOR_LR);

                    DrawRect(width, height);
                }
                DrawBlend(BLEND_ALPHA);
            }

            DrawFlush();
        }

        DrawEnd();

        t += dt*0.01f;
        deltaTimeU = PlatformTimeUsec() - startedTime;
        //fprintf(stderr, "t:%f\n", t);
    }

    DrawMeshTerminate(mesh);
    TextureTerminate(arena, texture);

    FontTerminate(font);
    FontTerminate(font2);

    //AudioSoundTerminate(sound);
    //AudioTerminate();

    AssetArchive(0, NULL);

    GraphicsTerminate();
    WindowTerminate(window);
    ArenaTerminate(arena);

    PlatformTerminate();

    return 0;
}


int LogTesting(void)
{
    PlatformInit();
    ARENA* arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);

    LogFrameBegin();

    // Do some formatting.
    String8 toFmt = STR8_LIT("Among us string: {u32:d}");

    LogInfo("String to format:");
    LogInfoStr(toFmt);
    LogInfo("Result:\n");
    Str8_format(arena, toFmt, 67);


    {
        String8 res = LogFrameEnd(arena, LOG_ALL, LOG_RES_CONCAT, TRUE);
        FastPrint(res);
    }

    PlatformTerminate();

    return 0;
}

int ExampleTesting(void)
{
    {
        LogFrameBegin();
    }

    //PlatformInit();

    ARENA* arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);
    Window* window = WindowInit(L"Example window", 640, 480);
    GraphicsInit(window);

    {
        String8 res = LogFrameEnd(arena, LOG_ALL, LOG_RES_CONCAT, TRUE);
        FastPrint(res);
    }

    // Upload mesh.
    float vertexData[] = {
        // x, y, z, nx, ny, nz, u, v, r, g, b, a.
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.5f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f,
    };

    int indices[] = {
        0, 1, 2,
    };

    //GFX_mesh* mesh = DrawUploadMesh(arena, vertexData, indices, 3);

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

    ArenaTerminate(arena);

    return 0;
}

int main()
{
    //LogTesting();
    //ExampleTesting();
    WindowTesting();

    return 0;
}

