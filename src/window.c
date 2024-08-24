#include "./window.h"

#include <SDL_video.h>
#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <GL/glew.h>

struct window {
    SDL_Window* window;
    SDL_GLContext context;
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

    SDL_GL_SetSwapInterval(0);

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