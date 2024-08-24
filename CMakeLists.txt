cmake_minimum_required(VERSION 3.12)
project(rudyscung C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

set(SOURCES
    src/render/camera.c
    src/render/chunk_renderer.c
    src/render/font.c
    src/render/level_renderer.c
    src/render/renderer.c
    src/render/shader.c
    src/render/shaders.c
    src/render/tessellator.c
    src/render/textures.c
    src/render/tile_renderer.c
    src/render/tile_texture_dispatcher.c
    src/world/chunk.c
    src/world/level.c
    src/world/side.c
    src/world/tile_shape.c
    src/world/tile.c
    src/main.c
    src/util.c
    src/window.c
)

add_executable(rudyscung ${SOURCES})

find_package(GLEW REQUIRED)
target_link_libraries(rudyscung PRIVATE GLEW::GLEW)

if(TARGET SDL2::SDL2main)
    target_link_libraries(rudyscung PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(rudyscung PRIVATE SDL2::SDL2)

find_package(SDL2_image REQUIRED)
target_link_libraries(rudyscung PRIVATE SDL2_image::SDL2_image)

find_package(OpenGL REQUIRED)
target_link_libraries(rudyscung PRIVATE OpenGL::GL)

target_link_libraries(rudyscung PRIVATE m)