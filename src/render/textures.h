#pragma once

#include <stdint.h>

#include "src/util/gl.h"

typedef struct texture {
    GLuint const name;
    size_t const size[2];
    size_t const num_channels;
    uint8_t const* const pixels;
} texture_t;

typedef enum texture_name {
    TEXTURE_NAME__TERRAIN,
    TEXTURE_NAME__FONT_DEFAULT,
    NUM_TEXTURE_NAMES
} texture_name_t;

typedef struct textures textures_t;

textures_t* const textures_new(char const* const resources_path);

void textures_delete(textures_t* const self);

void texture_delete(texture_t* const self);

texture_t const* const textures_get_texture(textures_t* const self, texture_name_t const texture_name);

texture_t* const textures_load_texture(textures_t const* const self, char const* const path);

uint32_t const texture_get_pixel(texture_t const* const self, size_t const pos[2]);
