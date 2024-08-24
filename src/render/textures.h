#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

typedef enum texture_name {
    TEXTURE_NAME__TERRAIN,
    TEXTURE_NAME__FONT_DEFAULT,
    NUM_TEXTURE_NAMES
} texture_name_t;

typedef struct textures textures_t;

textures_t* const textures_new(char const* const resources_path);

void textures_delete(textures_t* const self);

GLuint textures_get_texture(textures_t* const self, texture_name_t const texture_name);

// Note - DOES NOT CACHE TEXTURES
GLuint textures_get_texture_by_path(textures_t const* const self, char const* const texture_path);