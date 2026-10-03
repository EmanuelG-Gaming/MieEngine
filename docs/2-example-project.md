# 2. Example Project

To get started with MieEngine, you have
to set up your development environment.

MieEngine was primarily made to work on Windows, through
DirectX, but it's possible to get the engine running
on Linux and other UNIX operating systems, by using Wine.

This tutorial assumes that you are gonna use MSVC or some other C++ compiler
that can link the d3d9.lib files.

For MSVC, you gonna download this compiler by using Visual Studio.
Visual Studio is the IDE made by Microslohp, which is used for C++, but can also
be used for other languages like C# (through the .NET runtime).

The newest version is Visual Studio 2026, but it's preferred to use VS2022,
as the latest versions have more bloat.

When you set up Visual Studio, you would check the "Use desktop development with C++" box.
This installs the MSVC compiler, but also the Windows SDK.

On older versions of Windows, there was the DirectX SDK, which was later merged
into this Windows SDK. It is located at `C:\Program Files (x86)\Windows Kits\`,
for Windows 8.1, 10, and 11.
What we care about are the `Include/` and `Lib/` directories,
which contain the headers and the library implementations, respectively.

There are also two important environment variables in Windows
that we have to keep in mind: the PATH and the LIB.

The PATH variable is used by the terminal shell to more easily
locate executable files. Without PATH, you would have to specify the full
path to those files, which is tedious.

The LIB variable is used by MSVC to locate the library files
that it would link against, like user32.lib, d3d9.lib, d3d11.lib, etc.

These variables can be set by going into Settings->Search->Modify user environment variables
in Windows.

## The project's code
```cpp
int main(void)
{
    // TODO: Not really needed.
    PlatformInit();

    ARENA* arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);
    Window* window = WindowInit(arena, L"Testing window", 640, 480);

    GraphicsInit(window);

    while (WindowOpened(window))
    {
        // Poll events.
        WindowProcessEvents(window);

        // RGB format: (0, 17, 85).
        DrawClear(0x001155);

        // Present the framebuffer to the screen.
        DrawEnd();
    }

    GraphicsTerminate();
    WindowTerminate(window);

    PlatformTerminate();

    ArenaTerminate(arena);

    return 0;
}
```

The `PlatformInit()` function initializes some platform-specific
utilities, like the performance counter, which is used
for calculating the delta time (the duration of a frame),
and also the log file.
This isn't really needed early on, but we would get to that point.

The arena allocator is used to quickly allocate
things in a linear way. When the capacity is reached,
the current region is chained with a new arena, forming a linked list.
This makes the arena grow in size, as more elements are added.

Arena allocators are simpler and usually faster, compared to
the C stdlib's `malloc()`, which has a deceivingly simple API,
but with lots of differences between implementations.

The problem with arena allocators is that
there's no easy way to remove an element in the middle of the arena.
You can only `pop()` a certain amount of bytes from the top,
or `clear()` up the arena entirely.

This means that arena allocators are often used when you want to
allocate many elements that you free up at once.

Anyways, the `WindowInit()` function shows you a window, having a certain title and size.

The `GraphicsInit()` function initializes the graphics subsystem,
which can use either of your favourite graphics libraries (OpenGL, DirectX).

The `WindowOpened()` function is pretty self-explanatory,
but when an event is triggered that tells Windows to close our window,
the window listens to that event, which sets a certain bit flag
of the Window struct, causing the main game loop to finish,
because `WindowOpened()` is now 0 (the value of "false").

In the game loop, we first poll for events,
we then set a fill color as the background,
and then we present the result to the screen.

And then we release memory after the game loop has ended.


Once you've done that, you would probably have a window with a dark blue background!

In the next chapter, we're gonna talk about some graphics techniques
that have been used over the years, and how MieEngine would handle some of them.
