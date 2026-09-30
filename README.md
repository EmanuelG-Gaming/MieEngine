# DXApplication
A minimalist game framework/engine written in C++,
with DirectX9 and no uCRT being used, allowing the application to have very small binary sizes.

# Compilation
Before going into how it's compiled, let me explain something: What is uCRT?

uCRT ("Universal C Runtime") is an application layer developed my Microsoft.
It's the component that allows C stdlib functions (like `malloc`, `fopen` or `printf`) to be called.
The reason why this runtime was made is because they wanted to make applications be able to be ran on different systems.

You can think of uCRT like GlibC or Musl on Linux.


MSVC does some weird alignment stuff with the executable, so the minimum size we can get is an 8KB file.

Along with vcruntime, uCRT is a 100KB component embedded in every C application you build with MSVC, or MinGW GCC compiler, by default.
This means that it is statically-linked, which makes the executable larger.
You can get to some pretty large executable sizes. like 100KB or 600KB or 1MB, even if you didn't do much.

If you use the `/MD` compiler argument in MSVC, it will only load the dynamic library `ucrtbase.dll`,
which drastically reduces the application's size, at the expense of more indirection for every C stdlib call.

Additionally, many Win32 and experienced game developers warn you not to use the C stdlib,
instead opting for a more "Freestanding" environment.
This means that the programmer has to implement equivalent C standard functions themselves,
but it also makes reverse engineering efforts easier, because it's a smaller executable binary.


The reason why this file is so small, is because we use dynamic linking.
This means that the essential core libraries, like `kernel32.lib`, `libcmt.lib`, `user32.lib`, or `gdi32.lib`,
would be loaded by the application at runtime.
The process of DLL loading involves linking the library, creating a new memory region for the library and then loading it.

Of course, modern OSes can optimize for multiple executables that run the same libraries,
so that the linker only has to specify the paths for the respective functions of those libraries.


# Compilation
```
cl src/main.cpp /Zi /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" /link user32.lib d3d9.lib d3dcompiler.lib dxgi.lib /SUBSYSTEM:CONSOLE /OUT:app.exe
```


```
cl src/main.cpp /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" /DRIVER:none /link /NODEFAULTLIB kernel32.lib libcmt.lib user32.lib gdi32.lib d3d9.lib d3dcompiler.lib dxgi.lib /ENTRY:customMain /OUT:app.exe
```

```
cl src/main.cpp /ZI /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" /DRIVER:none /link /NODEFAULTLIB kernel32.lib libcmt.lib user32.lib gdi32.lib d3d9.lib d3dcompiler.lib dxgi.lib /ENTRY:customMain /OUT:app.exe
```

And then you can run the application by simply doing `app.exe`.



