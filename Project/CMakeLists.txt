cmake_minimum_required(VERSION 3.10)
project(BvhRayTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD          11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# Na Linuxu/macOS-u: ako nije navedeno, gradi Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(src)

add_executable(raytracer
    src/main.cc
    src/external/stb_image.h
    src/aabb.h
    src/bvh.h
    src/camera.h
    src/color.h
    src/constant_medium.h
    src/hittable.h
    src/hittable_list.h
    src/interval.h
    src/material.h
    src/perlin.h
    src/quad.h
    src/ray.h
    src/rtw_stb_image.h
    src/rtweekend.h
    src/sphere.h
    src/texture.h
    src/vec3.h
)

if(MSVC)
    add_compile_options(/W4)
endif()

#----------------------------------------------------------------------------------------------
# Animation: camera fly-around of the final scene, shown in a GLFW window and saved as PNG
# frames. GLFW is taken from the system if installed (e.g. `brew install glfw`), otherwise it
# is downloaded and built automatically (needs internet on the first `cmake -B ...`).
#----------------------------------------------------------------------------------------------
option(BUILD_ANIMATION "Build the GLFW camera animation (animation)" ON)

if(BUILD_ANIMATION)
    cmake_minimum_required(VERSION 3.14)   # FetchContent_MakeAvailable

    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
    find_package(glfw3 3.3 QUIET)

    if(NOT glfw3_FOUND)
        message(STATUS "GLFW not found on the system, downloading GLFW 3.4")
        include(FetchContent)
        if(POLICY CMP0135)
            cmake_policy(SET CMP0135 NEW)   # timestamps of downloaded files = extraction time
        endif()
        set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw
            URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
        )
        FetchContent_MakeAvailable(glfw)
    endif()

    add_executable(animation
        src/animation.cc
        src/gl_display.h
        src/scenes.h
        src/camera.h
        src/color.h
        src/rtweekend.h
        src/external/stb_image_write.h
    )
    # Thread-safe random numbers (see rtweekend.h)
    target_compile_definitions(animation PRIVATE RTW_MULTITHREADED)
    target_link_libraries(animation PRIVATE glfw OpenGL::GL Threads::Threads)

    # Interactive viewer with temporal (progressive) accumulation. Sequential (one thread),
    # uses std::rand() like raytracer, so it is NOT built with RTW_MULTITHREADED.
    add_executable(interactive
        src/interactive.cc
        src/gl_display.h
        src/scenes.h
        src/camera.h
        src/color.h
        src/rtweekend.h
        src/external/stb_image_write.h
    )
    target_link_libraries(interactive PRIVATE glfw OpenGL::GL)
endif()
