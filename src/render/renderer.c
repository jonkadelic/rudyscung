#include "./renderer.h"

#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "./level_renderer.h"

static void check_errors(void);

struct renderer {
    rudyscung_t* rudyscung;
    level_renderer_t* level_renderer;
};

renderer_t* const renderer_new(rudyscung_t* const rudyscung) {
    assert(rudyscung != nullptr);

    renderer_t* const self = malloc(sizeof(renderer_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;
    self->level_renderer = nullptr;

    return self;
}

void renderer_delete(renderer_t* const self) {
    assert(self != nullptr);

    free(self);
}

level_renderer_t* const renderer_get_level_renderer(renderer_t* const self) {
    assert(self != nullptr);

    return self->level_renderer;
}

void renderer_tick(renderer_t* const self) {
    assert(self != nullptr);

    if (self->level_renderer != nullptr) {
        level_renderer_tick(self->level_renderer);
    }
}

void renderer_set_level(renderer_t* const self, level_t const *const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    if (self->level_renderer != nullptr) {
        level_renderer_delete(self->level_renderer);
        self->level_renderer = nullptr;
    }

    self->level_renderer = level_renderer_new(self->rudyscung, level);
}

void renderer_render(renderer_t* const self, camera_t const* const camera) {
    assert(self != nullptr);

    level_renderer_draw(self->level_renderer, camera);

    check_errors();
}

static void check_errors(void) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error: %s\n", glewGetErrorString(error));
    }
}