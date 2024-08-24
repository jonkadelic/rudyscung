#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

typedef enum texture_name {
    TEXTURE_NAME__TERRAIN,
    NUM_TEXTURE_NAMES
} texture_name_t;

typedef struct textures textures_t;

textures_t* const textures_new(char const* const resources_path);

GLuint textures_get_texture(textures_t* const self, texture_name_t const texture_name);