#include "./textures.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "src/util/gl.h"
#include "lib/stb_image.h"
#include "../util.h"
#include "../util/logger.h"

struct textures {
    char const* resources_path;
    texture_t const* textures[NUM_TEXTURE_NAMES];
};

char const* const TEXTURE_NAME_LOOKUP[NUM_TEXTURE_NAMES] = {
    [TEXTURE_NAME__TERRAIN] = "/terrain.png",
    [TEXTURE_NAME__FONT_DEFAULT] = "/font/default.png"
};

textures_t* const textures_new(char const* const resources_path) {
    assert(resources_path != nullptr);

    textures_t* const self = malloc(sizeof(textures_t));
    assert(self != nullptr);
    self->resources_path = resources_path;

    for (size_t i = 0; i < NUM_TEXTURE_NAMES; i++) {
        self->textures[i] = nullptr;
    }

    LOG_DEBUG("textures_t: initialized.");

    return self;
}

void textures_delete(textures_t* const self) {
    assert(self != nullptr);

    for (texture_name_t tex = 0; tex < NUM_TEXTURE_NAMES; tex++) {
        if (self->textures[tex] != nullptr) {
            glDeleteTextures(1, &(self->textures[tex]->name));
            free((void*) self->textures[tex]->pixels);
            free((void*) self->textures[tex]);
        }
    }

    free(self);

    LOG_DEBUG("textures_t: deleted.");
}

void texture_delete(texture_t* const self) {
    assert(self != nullptr);

    glDeleteTextures(1, &(self->name));
    free((void*) self->pixels);
    free((void*) self);

    LOG_DEBUG("texture_t: deleted.");
}

texture_t const* const textures_get_texture(textures_t* const self, texture_name_t const texture_name) {
    assert(self != nullptr);
    assert(texture_name >= 0 && texture_name < NUM_TEXTURE_NAMES);

    texture_t const* tex = self->textures[texture_name];
    if (tex == nullptr) {
        char const* const texture_name_str = TEXTURE_NAME_LOOKUP[texture_name];
        texture_t const* const rw_tex = textures_load_texture(self, texture_name_str);
        self->textures[texture_name] = rw_tex;
        tex = rw_tex;

        LOG_DEBUG("textures_t: cached texture at \"%s\".", texture_name_str);
    }

    return tex;
}

texture_t* const textures_load_texture(textures_t const* const self, char const* const path) {
    assert(self != nullptr);
    assert(path != nullptr);

    char* real_path = strcata(self->resources_path, path);

    // Load texture
    int width, height, num_channels;
    uint8_t* const data = stbi_load(real_path, &width, &height, &num_channels, 0);
    assert(data != nullptr);

    free(real_path);

    GLuint mode = 0;
    switch (num_channels) {
        case 3:
            mode = GL_RGB;
            break;
        case 4:
            mode = GL_RGBA;
            break;
        default:
            assert(false);
    }

    // Generate texture in OpenGL
    GLuint name = 0;
    glGenTextures(1, &name);
    assert(name != 0);
    glBindTexture(GL_TEXTURE_2D, name);

    glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    texture_t const texture_local = (texture_t) {
        .name = name,
        .num_channels = num_channels,
        .size = { width, height },
        .pixels = data
    };
    texture_t* const texture = malloc(sizeof(texture_t));
    assert(texture != nullptr);

    memcpy(texture, &texture_local, sizeof(texture_t));

    return texture;
}

uint32_t const texture_get_pixel(texture_t const* const self, size_t const pos[2]) {
    assert(self != nullptr);
    assert(pos[0] < self->size[0]);
    assert(pos[1] < self->size[1]);

    uint32_t result = 0xFFFFFF;
    memcpy(&result, &(self->pixels[(pos[1] * self->size[1] + pos[0]) * self->num_channels]), self->num_channels);

    return result;
}
