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

These variables can be set by going into Settings->Search->Modify user environment variables.

