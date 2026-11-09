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
    src/external/stb_image_write.h
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
    src/rtw_stb_image_write.h
    src/rtweekend.h
    src/sphere.h
    src/texture.h
    src/vec3.h
)

if(MSVC)
    add_compile_options(/W4)
endif()