#include "./textures.h"

#include <SDL_surface.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <GL/gl.h>

#include "../util.h"

static GLuint load_texture(textures_t* const self, texture_name_t const texture_name);

struct textures {
    char const* resources_path;
    GLuint textures[NUM_TEXTURE_NAMES];
};

char const* const TEXTURE_NAME_LOOKUP[NUM_TEXTURE_NAMES] = {
    [TEXTURE_NAME__TERRAIN] = "/terrain.png"
};

static bool img_initialized = false;

textures_t* const textures_new(char const* const resources_path) {
    assert(resources_path != nullptr);

    textures_t* const self = malloc(sizeof(textures_t));
    assert(self != nullptr);
    self->resources_path = resources_path;

    for (size_t i = 0; i < NUM_TEXTURE_NAMES; i++) {
        self->textures[i] = 0;
    }

    // Initialize SDL_image
    if (!img_initialized) {
        int img_flags = IMG_INIT_PNG;
        int img_initted = IMG_Init(img_flags);
        assert((img_initted & img_flags) == img_flags);
        img_initialized = true;
    }

    return self;
}

GLuint textures_get_texture(textures_t* const self, texture_name_t const texture_name) {
    GLuint tex = self->textures[texture_name];
    if (tex == 0) {
        tex = load_texture(self, texture_name);
        self->textures[texture_name] = tex;
    }

    return tex;
}

static GLuint load_texture(textures_t* const self, texture_name_t const texture_name) {
    assert(self != nullptr);
    assert(texture_name >= 0 && texture_name < NUM_TEXTURE_NAMES);

    char const* const texture_name_str = TEXTURE_NAME_LOOKUP[texture_name];

    char* path = strcata(self->resources_path, texture_name_str);

    // Load texture using SDL
    SDL_Surface* surface = IMG_Load(path);
    assert(surface != nullptr);
    free(path);

    // Generate texture in OpenGL
    GLuint tex = 0;
    glGenTextures(1, &tex);
    assert(tex != 0);
    glBindTexture(GL_TEXTURE_2D, tex);

    uint8_t* pixels = surface->pixels;
    GLuint mode = GL_RGB;
    if (surface->format->BytesPerPixel == 4) {
        mode = GL_RGBA;
    } else if (surface->format->BytesPerPixel == 1) {
        mode = GL_RGBA;
        pixels = malloc(surface->w * surface->h * 4);
        assert(pixels != nullptr);
        for (int i = 0; i < surface->w * surface->h; i++) {
            uint8_t index = ((uint8_t*)(surface->pixels))[i];
            pixels[i * 4 + 0] = surface->format->palette->colors[index].r;
            pixels[i * 4 + 1] = surface->format->palette->colors[index].g;
            pixels[i * 4 + 2] = surface->format->palette->colors[index].b;
            pixels[i * 4 + 3] = surface->format->palette->colors[index].a;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, pixels);
    if (surface->format->BytesPerPixel == 1) {
        free(pixels);
    }
    SDL_FreeSurface(surface);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return tex;
}