#include "./renderer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#include "src/render/gl.h"
#include "src/client/client.h"
#include "src/render/level_renderer.h"
#include "src/client/window.h"
#include "src/render/font.h"
#include "src/phys/raycast.h"
#include "src/util/logger.h"

static void check_errors(void);
static void draw_overlay(renderer_t* const self);

struct renderer {
    client_t* client;
    level_renderer_t* level_renderer;
    struct {
        size_t target_fps;
        float fps;
        uint64_t last_frame_tick;
        uint64_t last_fps_update_tick;
        size_t frames_since_last_fps_update;
    } frames;
};

renderer_t* const renderer_new(client_t* const client) {
    assert(client != nullptr);

    renderer_t* const self = malloc(sizeof(renderer_t));
    assert(self != nullptr);

    self->client = client;
    self->level_renderer = nullptr;
    self->frames.target_fps = 60;
    self->frames.last_frame_tick = SDL_GetTicks64();
    self->frames.last_fps_update_tick = SDL_GetTicks64();
    self->frames.frames_since_last_fps_update = 0;

    LOG_DEBUG("renderer_t: initialized.");

    return self;
}

void renderer_delete(renderer_t* const self) {
    assert(self != nullptr);

    if (self->level_renderer != nullptr) {
        level_renderer_delete(self->level_renderer);
    }

    free(self);

    LOG_DEBUG("renderer_t: deleted.");
}

void renderer_level_changed(renderer_t* const self) {
    assert(self != nullptr);

    if (self->level_renderer == nullptr) {
        self->level_renderer = level_renderer_new(self->client);
    } else {
        level_renderer_level_changed(self->level_renderer);
    }
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

void renderer_render(renderer_t* const self, camera_t* const camera, float const partial_tick) {
    assert(self != nullptr);

    uint64_t tick_start = SDL_GetTicks64();

    window_t const* const window = client_get_window(self->client);

    // Render
    if (self->frames.target_fps == 0 || ((tick_start - self->frames.last_frame_tick) > (1000.0f / (self->frames.target_fps + 3)))) {
        glClearColor(0.2f, 0.5f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (self->level_renderer != nullptr) {
            level_renderer_draw(self->level_renderer, camera, partial_tick);
        }

        draw_overlay(self);

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

static void draw_overlay(renderer_t* const self) {
    assert(self != nullptr);

    font_t const* const font = client_get_font(self->client);

    char line_buffer[64];
    snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "FPS: %.2f", self->frames.fps);
    font_draw(font, line_buffer, 0, 0);

    if (self->level_renderer != nullptr) {
        level_t* const level = client_get_level(self->client);
        ecs_t* const ecs = level_get_ecs(level);
        entity_t const player = client_get_player(self->client);

        ecs_component_pos_t const* const player_pos = ecs_get_component_data(ecs, player, ECS_COMPONENT__POS);
        ecs_component_vel_t const* const player_vel = ecs_get_component_data(ecs, player, ECS_COMPONENT__VEL);
        ecs_component_rot_t const* const player_rot = ecs_get_component_data(ecs, player, ECS_COMPONENT__ROT);

        snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "x: %.2f y: %.2f z: %.2f", player_pos->pos[AXIS__X], player_pos->pos[AXIS__Y], player_pos->pos[AXIS__Z]);
        font_draw(font, line_buffer, 0, 12);

        snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "vx: %.2f vy: %.2f z: %.2f", player_vel->vel[AXIS__X], player_vel->vel[AXIS__Y], player_vel->vel[AXIS__Z]);
        font_draw(font, line_buffer, 0, 24);

        raycast_t raycast;
        raycast_cast_in_level(&raycast, level, player_pos->pos, player_rot->rot);
        if (raycast.hit) {
            snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "hit: %zu %zu %zu, block: %d", raycast.tile_pos[AXIS__X], raycast.tile_pos[AXIS__Y], raycast.tile_pos[AXIS__Z], raycast.tile);
            font_draw(font, line_buffer, 0, 36);
        }
    }
}