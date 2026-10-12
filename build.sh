#!/bin/bash

# MSVC compiler arguments are difficult to handle in UNIX-style shell scripts.

# The C++ minimum version is C++14, since it improves Constexpr behaviour.


TARGETS=()

COMPILER="clang"
TARGET_FLAG="none"
GRAPHICS="d3d11"
BUILD_MODE="release"

function ValidateNextArgument {
    if [[ -z "$2" || "$2" == -* ]]; then
        echo "Option $1 needs a value!"
        exit 1
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        "-a"|"--all"|"all")
            TARGET_FLAG="all"
            shift
        ;;

        "content")
            TARGET_FLAG="content"
            shift
        ;;

        "-c"|"--compiler")
            ValidateNextArgument "$1" "$2"

            COMPILER="$2"
            TARGET_FLAG="false"
            shift # Shifts cursor to the right, past the argument.
            shift # Past value.
        ;;

        "-g"|"--graphics")
            ValidateNextArgument "$1" "$2"

            GRAPHICS="$2"
            TARGET_FLAG="false"
            shift
            shift
        ;;

        "-b"|"--build")
            ValidateNextArgument "$1" "$2"

            BUILD_MODE="$2"
            TARGET_FLAG="false"
            shift
            shift
        ;;

        "-t"|"--target"|"--targets")
            ValidateNextArgument "$1" "$2"

            if [ "$2" = "all" ]; then
                # Use for all targets.
                TARGET_FLAG="all"
            elif [ "%2" = "content" ]; then
                # Compile shaders and scripts.
                TARGET_FLAG="content"
            else
                # Use for specific targets.
                TARGET_FLAG="true"
            fi

            shift # Past argument
        ;;

        -*|--*)
            echo "Unknown option: $1"
            exit 1
        ;;

        *)
            if [ "$TARGET_FLAG" == "true" ]; then
                TARGETS+=("$1") # Save positional arg.
            else
                echo "Unexpected positional argument: $1"
                exit 1
            fi

            shift
        ;;
    esac
done


if [ "$TARGET_FLAG" = "none" ]; then
    echo "No target flags specified!"
    echo "Usage:"
    echo "build -t, --target/s <Args>"
    echo "With either 'engine', 'archiver_compile', 'shaders', 'scripts', or 'archive'."
    echo ""
    echo "Special flags:"
    echo "'content' -> Compiles external content and packs it into an archive file."
    echo "'all'     -> Compiles every aforementioned unit."
    echo ""
    exit 0
fi

if [ "$TARGET_FLAG" = "content" ]; then
    TARGETS+=("shaders", "scripts", "archive")
elif [ "$TARGET_FLAG" = "all" ]; then
    TARGETS+=("engine", "archiver_compile", "shaders", "scripts", "archive")
fi



set -- "${TARGETS[@]}" # Restore positional args.


# Read arguments.
#if [ $# -lt 1 ]; then
#    COMPILER="msvc"
#else
#    COMPILER="$1"
#fi

#if [ $# -lt 2 ]; then
#    TARGETS="all"
#else
#    TARGETS=(${2//;/})
#fi

#if [ $# -lt 3 ]; then
#    GRAPHICS="d3d11"
#else
#    GRAPHICS="$3"
#fi



#set -e

echo "Compiling..."

if [ "$COMPILER" = "msvc" ]; then

# Windows MSVC build:
# Requires:
# Visual Studio for MSVC and Windows SDK.

CXX=cl.exe
CXX_FLAGS="$CXX_FLAGS -std:c++14"
if [ "$BUILD_MODE" = "release" ]; then
    CXX_FLAGS+=" -O1"
elif [ "$BUILD_MODE" = "regular" ]; then
    :
elif [ "$BUILD_MODE" = "debug" ]; then
    CXX_FLAGS+=" -Zi -DEBUG"
else
    echo "Incorrect build mode!"
    exit 1
fi

CXX_INCLUDES=\
    "-IC:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" \
    "-IC:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared" \
    "-IC:\Program Files\Microsoft Visual Studio/2022\Community/VC\Tools\MSVC\14.44.35207\include" \
    "-IC:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt"
CXX_LINKS="-link kernel32.lib libcmt.lib user32.lib gdi32.lib $GRAPHICS.lib dxgi.lib"
CXX_LINKS="$CXX_LINKS -SUBSYSTEM:CONSOLE"

CXX_OUTFLAG="/OUT:"

elif [ "$COMPILER" = "clang" ]; then

# Windows Clang build:
# Requires:
# MSYS2/Visual Studio for Clang
# If you're using Visual Studio, you would have Windows SDK
# installed using the C++ desktop Development package.

CXX=clang++
CXX_FLAGS="$CXX_FLAGS -std=c++14"
if [ "$BUILD_MODE" = "release" ]; then
    CXX_FLAGS+=" -O3"
elif [ "$BUILD_MODE" = "regular" ]; then
    :
elif [ "$BUILD_MODE" = "debug" ]; then
    CXX_FLAGS+=' -g'
else
    echo "Incorrect build mode!"
    exit 1
fi

CXX_INCLUDES=""
CXX_LINKS="-lkernel32 -luser32 -lgdi32 -ldsound -ldxguid -lwinmm -l$GRAPHICS -ldxgi" # To remove dxguid and winmm.

CXX_OUTFLAG="-o"
fi


set -x

# Compile targets now.
# The build recipes must be executed in order:
# engine, asset_archiver -> These are compiled executables
# then
# shader_compilation, archive_files. -> These are for tools to be used (fxc, asset_archiver).

# We can do this by pattern matching the target array with specific strings.

if [[ "${TARGETS[*]}" =~ "engine" ]]; then
    CXX_SRCPATH="src/main.cpp"
    BUILD_BIN="app.exe"
    $CXX $CXX_SRCPATH $CXX_FLAGS $CXX_INCLUDES $CXX_LINKS $CXX_OUTFLAG $BUILD_BIN
fi

if [[ "${TARGETS[*]}" =~ "archiver_compile" ]]; then
    CXX_SRCPATH="tools/asset_archiver.cpp"
    BUILD_BIN="archiver.exe"
    $CXX $CXX_SRCPATH $CXX_FLAGS $CXX_INCLUDES $CXX_LINKS $CXX_OUTFLAG $BUILD_BIN
fi

if [[ "${TARGETS[*]}" =~ "shaders" ]]; then
    # Compile shaders now.
    # Requires d3dcompiler.dll and fxc.exe as its cmdline interface.
    if [ "$GRAPHICS" = "d3d9" ] || [ "$GRAPHICS" = "d3d11" ]; then
        cd shaders
        for file in *.hlsl; do
            if [ "-f $file" ]; then
                ./fxc.exe -T vs_5_0 -E VS -Fo "../assets/shaders/${file%%.*}.vso" "$file"
                echo ""
                ./fxc.exe -T ps_5_0 -E PS -Fo "../assets/shaders/${file%%.*}.pso" "$file"
                echo ""
            fi
        done
        cd ..
    else
        echo "Unknown graphics library for shader compilation!"
    fi
fi

if [[ "${TARGETS[*]}" =~ "archive" ]]; then
    # Do some asset archiving.
    ./archiver.exe
fi




set +x



exit
