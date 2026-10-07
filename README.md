# MieEngine
A minimalist game framework/engine written in C++,
It can choose between one of the graphics libraries that it can use.

# Characteristics
- State machine-based batched renderer.
- Animation system.
- Bitmap and TTF text.
- Audio.
- Asset archiving.


# Compilation
To compile this project, you're gonna need any C++ compiler
that can link binaries, and also the library set of Windows SDK.

## Windows
On Windows, you're gonna probably use MSVC (`cl.exe`) as the compiler.

MSVC can be installed from VS2026 (you can also use VS2022 from a R\*ddit post).
You can also use Clang, or Clang-cl, which come with Visual Studio's toolchain
from the C/C++ desktop development package.

Now, once you've installed Visual Studio, you might see that it has
the `C:\Program Files\Microsoft Visual Studio\<version>` directory
being placed. You might also see the Windows SDK being installed,
which is located at `C:\Program Files (x86)\Windows Kits\10`.

The Windows SDK is a replacement for the older DirectX SDK.


To compile with MSVC on the terminal, you would need
the `PATH` environment variable, so that the shell can locate executables more easily.

MSVC uses the `LIB` environment variable to locate library `.lib` files,
so that they can be linked to the project.


To compile the main engine, you're gonna add this command to the terminal.
```
cl src/main.cpp /ZI /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" /I "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include" /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt" /DRIVER:none /link /NODEFAULTLIB kernel32.lib libcmt.lib user32.lib gdi32.lib d3d9.lib d3dcompiler.lib dxgi.lib /ENTRY:customMain /OUT:app.exe`
```


More details can be found in the [Documentation](https://github.com/EmanuelG-Gaming/MieEngine/blob/main/docs/2-example-project.md).



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



