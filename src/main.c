#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>

#include "./window.h"
#include "render/font.h"
#include "render/renderer.h"
#include "render/shaders.h"
#include "render/textures.h"
#include "render/camera.h"
#include "util.h"
#include "world/chunk.h"
#include "world/level.h"
#include "world/tile.h"

#define PROJECT_NAME "rudyscung"

#define TICKS_PER_SECOND 20
#define MS_PER_TICK (1000 / (TICKS_PER_SECOND))

static struct {
    bool w;
    bool s;
    bool a;
    bool d;
    bool left;
    bool right;
    bool up;
    bool down;
    bool space;
    bool shift;
} keys;

static void tick(camera_t* const camera);

int main(int argc, char** argv) {
    window_t* window = window_new(PROJECT_NAME, 800, 600);

    char const* resources_path = "../res";
    if (argc > 1) {
        resources_path = argv[1];
    }

    tiles_init();
    shaders_init(resources_path);

    textures_t* textures = textures_new(resources_path);
    renderer_t* renderer = renderer_new(window, textures);
    font_t* font = font_new(textures, resources_path, "default");

#define LEVEL_SIZE 4
    level_t* level = level_new(LEVEL_SIZE, 4, LEVEL_SIZE);
    for (size_chunks_t x = 0; x < LEVEL_SIZE; x++) {
        for (size_chunks_t y = 0; y < 4; y++) {
            for (size_chunks_t z = 0; z < LEVEL_SIZE; z++) {
                if ((y * CHUNK_SIZE) < 48) {
                    level_set_tile(level, x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE, tile_get(TILE_ID__GRASS));
                }
            }
        }
    }

    renderer_set_level(renderer, level);

    camera_t* camera = camera_new(-32.0f, -70.0f, -100.0f);

    bool running = true;
    uint64_t last_tick = SDL_GetTicks64();
    uint64_t last_fps_tick = SDL_GetTicks64();
    size_t frames = 0;
    size_t fps = 0;
    while (running) {
        uint64_t tick_start = SDL_GetTicks64();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool is_pressed = event.type == SDL_KEYDOWN;
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        keys.w = is_pressed;
                        break;
                    case SDLK_s:
                        keys.s = is_pressed;
                        break;
                    case SDLK_a:
                        keys.a = is_pressed;
                        break;
                    case SDLK_d:
                        keys.d = is_pressed;
                        break;
                    case SDLK_LEFT:
                        keys.left = is_pressed;
                        break;
                    case SDLK_RIGHT:
                        keys.right = is_pressed;
                        break;
                    case SDLK_UP:
                        keys.up = is_pressed;
                        break;
                    case SDLK_DOWN:
                        keys.down = is_pressed;
                        break;
                    case SDLK_SPACE:
                        keys.space = is_pressed;
                        break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                        keys.shift = is_pressed;
                        break;
                }
            }
        }
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer_render(renderer, camera);
        char fps_buffer[32];
        snprintf(fps_buffer, sizeof(fps_buffer) / sizeof(fps_buffer[0]), "FPS: %zu", fps);
        font_draw(font, fps_buffer, 0, 0);

        window_swap(window);

        uint64_t current_tick = SDL_GetTicks64();
        uint64_t delta_tick = current_tick - last_tick;
        size_t ticks = (size_t)(delta_tick / MS_PER_TICK);
        for (uint64_t t = 0; t < ticks; t++) {
            tick(camera);
            renderer_tick(renderer);
        }
        last_tick = current_tick - (delta_tick % MS_PER_TICK);

        uint64_t tick_end = SDL_GetTicks64();
        uint64_t frame_delta = tick_end - last_fps_tick;
        if (frame_delta > 1000) {
            fps = (size_t)(frames / (frame_delta / 1000.0f));
            last_fps_tick = tick_end;
            frames = 0;
        } else {
            frames++;
        }
    }

    renderer_delete(renderer);
    window_delete(window);

    return 0;
}

static void tick(camera_t* const camera) {
    float camera_pos[3];
    camera_get_pos(camera, camera_pos);
    float camera_rot[2];
    camera_get_rot(camera, camera_rot);

    float pos_dx = 0;
    float pos_dy = 0;
    float pos_dz = 0;
    float rot_dy = 0;
    float rot_dx = 0;

    if (keys.w) {
        pos_dz++;
    }
    if (keys.s) {
        pos_dz--;
    }
    if (keys.a) {
        pos_dx--;
    }
    if (keys.d) {
        pos_dx++;
    }
    if (keys.left) {
        rot_dy -= 0.01;
    }
    if (keys.right) {
        rot_dy += 0.01;
    }
    if (keys.up) {
        rot_dx -= 0.01;
    }
    if (keys.down) {
        rot_dx += 0.01;
    }
    if (keys.space) {
        pos_dy++;
    }
    if (keys.shift) {
        pos_dy--;
    }

    camera_set_pos(camera, camera_pos[0] + pos_dx, camera_pos[1] + pos_dy, camera_pos[2] + pos_dz);
    camera_set_rot(camera, camera_rot[0] + rot_dy, camera_rot[1] + rot_dx);
}