cmake_minimum_required(VERSION 3.0.0...3.13)


# target_include_directories(MieEngine PUBLIC "src")

set(ENGINE_HEADERS
    # HEADERS

    ${EXTERNAL_HEADERS}

    # Base.
    base/base_defs.h
    base/base_string.h
    base/base_fmt.h
    base/base_log.h

    base/math/base_rng.h
    base/math/mathf.h
    base/math/mathf_fixed.h
    base/math/mathf_wrap.h
    base/math/base_noise.h

    # IO.
    io/asset.h

    # Memory.
    mem/arena.h

    # Platform.
    platform/platform.h

    # Windowing.
    win/win.h

    # Core.
    core/interp.h
    core/timer.h

    # Graphics.
    graphics/draw.h
    graphics/mesh.h
    graphics/texture.h
    graphics/camera.h

    graphics/image/image.h
    graphics/font/font.h

    # Audio.
    audio/audio.h

    # Runtime.
    runtime/anm_loaded.h
    runtime/anm_manager.h
    runtime/anm_vars.h
    runtime/anm_vm.h
)

# SOURCES
target_sources(MieEngine PRIVATE
    base/math/base_rng.cpp
    base/math/base_noise.cpp
    base/math/mathf.cpp
    base/math/mathf_fixed.cpp

    base/base_log.cpp
    base/base_fmt.cpp
    base/base_string.cpp

    # Core.
    core/interp.cpp
    core/timer.cpp

    # Memory.
    mem/arena.cpp


    # IO.
    io/asset.cpp

    # Platform.
    platform/platform.cpp

    # Windowing.
    win/win32/window.cpp

    # Graphics.
    graphics/draw.cpp
    graphics/mesh.cpp
    graphics/texture.cpp
    graphics/camera.cpp

    graphics/graphics.cpp

    graphics/image/image.cpp
    graphics/font/font.cpp

    # Audio.
    audio/audio.cpp

    # Runtime.
    runtime/anm_loaded.cpp
    runtime/anm_manager.cpp
    runtime/anm_vm.cpp
)

# add_subdirectory("base")

#target_link_libraries(MieEngine "${D3D11_LIBRARY}")
#target_sources(MieEngine PRIVATE "${ENGINE_SOURCES}")

