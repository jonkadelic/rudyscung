#include "./window.h"

#include <SDL_video.h>
#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>

struct window {
    SDL_Window* window;
    SDL_GLContext context;
    float gui_scale;
};

window_t* const window_new(char const* const title, int const width, int const height) {
    window_t* const self = malloc(sizeof(window_t));
    assert(self != nullptr);

    self->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    assert(self->window != nullptr);

    self->context = SDL_GL_CreateContext(self->window);
    assert(self->context != nullptr);
    
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        assert(false);
    }

    self->gui_scale = 2.0f;

    // SDL_GL_SetSwapInterval(0);

    return self;
}

void window_delete(window_t* const self) {
    assert(self != nullptr);
    SDL_GL_DeleteContext(self->context);
    self->context = nullptr;
    SDL_DestroyWindow(self->window);
    self->window = nullptr;
    free(self);
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

    self->gui_scale = scale;
}
