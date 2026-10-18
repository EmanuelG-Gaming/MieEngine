# CMake.
#
# If you go down the Visual Studio route, then CMake would build it with
# MSVC and MSbuild.
# The compiled executable file would use MSVCrt instead of uCRT.
#
# Steps:
# Building the engine, building the tools, using the tools.
#
# Usage:
# Building the main engine:
# cmake .. && cmake --build .
#
# Building the archiver:
# cmake --build
#
# Using the shader compiler:
#
# Archiving the files:
#
# Getting the compile_commands.json file for LSPs:
# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 build
# 
# However, you cannot use CMake to generate the compile_commands.json
# file when you have MSVC (MSbuild).
# You have to use something like Makefile with GCC/Clang.
# Those are the ones with similar compiler arguments.
#
# cmake -G "Unix Makefiles" -B . -S .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1
#
# This is arguably one of the slowest build processes on Windows,
# mostly because it uses Clang/GCC and separate translation units per source file,
# but it's also the one that gets you the compile commands.
#
# A faster alternative to Makefiles is to use "Ninja".
#







cmake_minimum_required(VERSION 3.0.0...3.13)
project(MieEngine VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")


# For Windows building.
# (Either download legacy DirectX SDK or Windows SDK from Visual Studio)
# find_library(D3D9_LIBRARY d3d9.lib PATHS "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x86")
find_library(D3D11_LIBRARY d3d11.lib PATHS "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x86")
find_library(DXGI_LIBRARY dxgi.lib PATHS "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x86")



add_library(MieEngine STATIC)
target_include_directories(MieEngine PUBLIC "include")
target_link_libraries(MieEngine "${D3D11_LIBRARY}" "${DXGI_LIBRARY}")

target_link_libraries(${OUTPUT_NAME} MieEngine)


add_subdirectory("ext")


# Link libraries.
add_subdirectory("src")

# Add tools.
add_executable(archiver "tools/asset_archiver.cpp")



# Testing.
#add_executable(main "src/main.cpp")
#set_property(TARGET main PROPERTY CXX_STANDARD 14)
#target_link_libraries(main MieEngine)

# Use the tools.

# FXC compiler is located at shaders/fxc

#find_program(FXC fxc DOC "DirectX FXC compiler")
#if(NOT FXC)
#    message("Couldn't find fxc")
#endif(NOT FXC)


add_custom_target(Tools ALL)
add_custom_target(fx ALL)

foreach(FILE shaders/base.hlsl)
    get_filename_component(FILE_WE ${FILE} NAME_WE)

    foreach(VERTEX_TARGET VS)
        set(OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}-${VERTEX_TARGET}.vso")

        add_custom_command(
            OUTPUT ${OUT_FILE}
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/shaders/fxc" -T vs_5_0 -E ${VERTEX_TARGET}
                    -Fo "${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}-${VERTEX_TARGET}.vso" "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"

            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
            COMMENT "DX Vertex Compile ${FILE}"
            VERBATIM
        )
        #add_dependencies(fx "${FILE_WE}-${PIXEL_TARGET}.pso")
        target_sources(fx PRIVATE ${OUT_FILE})
    endforeach(VERTEX_TARGET)

    foreach(PIXEL_TARGET PS Distort_PS)
        set(OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}-${PIXEL_TARGET}.pso")
        add_custom_command(
            OUTPUT ${OUT_FILE}
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/shaders/fxc" -T ps_5_0 -E ${PIXEL_TARGET}
                    -Fo "${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}-${PIXEL_TARGET}.pso" "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"

            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}"
            COMMENT "DX Pixel Compile ${FILE}"
            VERBATIM
        )
        #add_dependencies(fx "${FILE_WE}-${PIXEL_TARGET}.pso") # What's expected.
        target_sources(fx PRIVATE ${OUT_FILE})
    endforeach(PIXEL_TARGET)
endforeach(FILE)

add_dependencies(Tools fx)




#set(OUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets.bin")
#add_custom_command(
#    OUTPUT "${OUT_FILE}"
#    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/archiver" "${CMAKE_CURRENT_SOURCE_DIR}/assets"
#    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets"
#    COMMENT "Archive assets"
#    VERBATIM
#)

#add_custom_target(ArchiveAssets ALL DEPENDS "${OUT_FILE}")
