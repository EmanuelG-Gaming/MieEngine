# MieEngine
This is a game engine that uses Direct3D 11 as the graphics library.


# Characteristics
- D3D11 renderer backend
- Custom allocators: Arena, memory pools.
- C++11-C++14 STL maximum (e.g.`<unordered_map>` for C++11)
- Lightweight VM runtime.

- Minimalism:
- C-like C++ codebase (raw C structs and functions, or C++ classes with encapsulation and no virtual functions).
- No CMake being used. Everything gets built using one compiler command.
Specialized tools can be used for things like asset packing.
- No vendored libraries (GLFW, RGFW, SDL, ImGUI, etc).
- Things implemented almost completely from scratch, only using the OS's native syscalls or the graphics library itself.
This does require more lines of code for the application side, but you trim off the excess kebab
that exists inside most libraries (you act as the compiler now, doing dead code elimination yourself).



# Building
This project has no build system like CMake or .sln files, mostly
because it uses unity builds to get the preprocessor to
place every header and source files into a single translation entry
to be sent in one compiler call.
This allows the text editor (Vim, Neovim, Emacs, etc) to be decoupled from the build system/toolchain,
but this example will use Visual Studio 2022.

You have to set up the Windows SDK directory, along with the DirectX SDK directory.
If you have Visual Studio installed with the Desktop building components for Windows,
there's already d3d11.lib, dxgi.lib, etc. along with the headers for windows.h, d3d11.h, etc.

The developer command prompt for VS2022 automatically sets up the
INCLUDE, LIB and PATH variables. This is done through a Batch script
located at `C:\<Path\to\VS2022>\Common7\Tools\VsDevCmd.bat`.
The problem with this script is that it also initializes quite some useless
things that slow down the startup times considerably.

VS2022 also no longer includes the legacy DirectX SDK.
Instead, DirectX headers and libraries are included in the Windows SDK kit,
which appears when you install VS2022.

So the DirectX headers (d3d11.h, dxgi.h) are located in:
```
C:\Program files (x86)\Windows Kits\10\include\<SDK VERSION>\um\
C:\Program files (x86)\Windows Kits\10\include\<SDK VERSION>\shared\
```

And the DirectX libraries are in:
```
C:\Program files (x86)\Windows Kits\10\Lib\<SDK VERSION>\um\<ARCH>\
```

So I would advise you to manually set up those aforementioned variables.

You would also have to locate the uCRT includes and MSVC's C++ includes.
MSVC has the C compatibility headers like cstdio, which maps C headers from uCRT, into equivalent C++ calls.

If you're using MSVC (cl.exe), you have to use this command:

```
cl src/main.cpp /Zi /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" /link user32.lib d3d11.lib d3dcompiler.lib dxgi.lib /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /OUT:app.exe
```

Add `/DEBUG` for debug builds, and `/O2` for release builds.
IF you want to show logs to the terminal (console mode),
you have to replace the '/SUBSYSTEM:WINDOWS' with '/SUBSYSTEM:CONSOLE', and remove the '/ENTRY' part.

Some of the quirks of the MSVC compiler, compared to things like GCC and Clang is that
MSVC's C++ mode is probably the strictest when it comes to type casting.
While other compilers allow for some implicit conversions (or they would only generate warnings).
MSVC demands the programmer to be extremely explicit.
MSVC applies the C++ standards about type compatibility in a very rigid way.

Declaring Compound Literals with parentheses (like `(vec3) { x, y, z }` style from C99) doesn't work.
It historically did not support C99 compound literals, so C++ mode generates an error.
MSVC C mode is only supported for C89 and C99;
Compound literals are a non-standard feature in C++, but they are widely supported by GCC and Clang as extensions.

MSVC from Visual Studio 2022 will use C++14 by default (`/std=C++14`), for compatibility with older projects.

Another quirk is that, of course, compiler extensions are different between MSVC and GCC.

# Availability
The code is available on Microslohp Github: https://github.com/EmanuelG-Gaming/MieEgine.
