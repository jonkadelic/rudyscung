#include "./window.h"

#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>

#include "src/render/gl.h"
#include "src/util/logger.h"

struct window {
    SDL_Window* window;
    SDL_GLContext context;
    float gui_scale;
};

window_t* const window_new(char const* const title, int const width, int const height) {
    window_t* const self = malloc(sizeof(window_t));
    assert(self != nullptr);

    SDL_Init(SDL_INIT_VIDEO);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    self->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    assert(self->window != nullptr);

    self->context = SDL_GL_CreateContext(self->window);
    assert(self->context != nullptr);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        LOG_ERROR("window_t: Failed to initialize GLEW!");
        assert(false);
    }

    self->gui_scale = 2.0f;

    // SDL_GL_SetSwapInterval(0);

    LOG_DEBUG("window_t: initialized.");

    return self;
}

void window_delete(window_t* const self) {
    assert(self != nullptr);
    SDL_GL_DeleteContext(self->context);
    self->context = nullptr;
    SDL_DestroyWindow(self->window);
    self->window = nullptr;
    free(self);

    LOG_DEBUG("window_t: deleted.");
}

void window_swap(window_t const* const self) {
    assert(self != nullptr);

    SDL_GL_SwapWindow(self->window);
}

void window_get_size(window_t const* const self, size_t size[2]) {
    assert(self != nullptr);

    int width, height;
    SDL_GetWindowSize(self->window, &width, &height);

    size[0] = width;
    size[1] = height;
}

void window_handle_resize(window_t* const self) {
    assert(self != nullptr);

    size_t size[2];
    window_get_size(self, size);

    glViewport(0, 0, size[0], size[1]);
}

float window_get_gui_scale(window_t const* const self) {
    assert(self != nullptr);

    return self->gui_scale;
}

void window_set_gui_scale(window_t* const self, float const scale) {
    assert(self != nullptr);

    float old_scale = self->gui_scale;

    self->gui_scale = scale;

    LOG_DEBUG("window_t: changed GUI scale (%.2f -> %.2f)", old_scale, scale);
}
