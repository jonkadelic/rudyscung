#include "./renderer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "../rudyscung.h"
#include "./level_renderer.h"
#include "../window.h"
#include "./font.h"

static void check_errors(void);

struct renderer {
    rudyscung_t* rudyscung;
    level_renderer_t* level_renderer;
    struct {
        size_t target_fps;
        float fps;
        uint64_t last_frame_tick;
        uint64_t last_fps_update_tick;
        size_t frames_since_last_fps_update;
    } frames;
};

renderer_t* const renderer_new(rudyscung_t* const rudyscung) {
    assert(rudyscung != nullptr);

    renderer_t* const self = malloc(sizeof(renderer_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;
    self->level_renderer = nullptr;
    self->frames.target_fps = 60;
    self->frames.last_frame_tick = SDL_GetTicks64();
    self->frames.last_fps_update_tick = SDL_GetTicks64();
    self->frames.frames_since_last_fps_update = 0;

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

    uint64_t tick_start = SDL_GetTicks64();

    window_t const* const window = rudyscung_get_window(self->rudyscung);
    font_t const* const font = rudyscung_get_font(self->rudyscung);

    // Render
    if (self->frames.target_fps == 0 || ((tick_start - self->frames.last_frame_tick) > (1000.0f / (self->frames.target_fps + 3)))) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        level_renderer_draw(self->level_renderer, camera);

        char line_buffer[64];
        snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "FPS: %.2f", self->frames.fps);
        font_draw(font, line_buffer, 0, 0);
        float camera_pos[NUM_AXES];
        camera_get_pos(camera, camera_pos);
        snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "x: %.2f y: %.2f z: %.2f", camera_pos[0], camera_pos[1], camera_pos[2]);
        font_draw(font, line_buffer, 0, 12);

        window_swap(window);

        self->frames.last_frame_tick = SDL_GetTicks64();

        uint64_t tick_end = SDL_GetTicks64();
        uint64_t frame_delta = tick_end - self->frames.last_fps_update_tick;
        if (frame_delta > 1000) {
            self->frames.fps = (size_t)(self->frames.frames_since_last_fps_update / (frame_delta / 1000.0f));
            self->frames.last_fps_update_tick = tick_end;
            self->frames.frames_since_last_fps_update = 0;
        } else {
            self->frames.frames_since_last_fps_update++;
        }
    }

    check_errors();
}

static void check_errors(void) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error: %s\n", glewGetErrorString(error));
    }
}