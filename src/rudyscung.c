#include "./rudyscung.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_events.h>
#include <SDL_timer.h>

#include "render/font.h"
#include "render/shaders.h"
#include "render/textures.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "world/level.h"

#define WINDOW_TITLE "rudyscung"
#define WINDOW_INITIAL_WIDTH 800
#define WINDOW_INITIAL_HEIGHT 600

#define TICKS_PER_SECOND 20
#define MS_PER_TICK (1000 / (TICKS_PER_SECOND))


struct rudyscung {
    window_t* window;
    shaders_t* shaders;
    textures_t* textures;
    font_t* font;
};

// TODO: Replace with actual input handling
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

rudyscung_t* const rudyscung_new(char const* const resources_path) {
    assert(resources_path != nullptr);

    rudyscung_t* self = malloc(sizeof(rudyscung_t));
    assert(self != nullptr);

    self->window = window_new(WINDOW_TITLE, WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT);
    self->shaders = shaders_new(resources_path);
    self->textures = textures_new(resources_path);
    self->font = font_new(self, resources_path, "default");

    return self;
}

void rudyscung_delete(rudyscung_t* const self) {
    assert(self != nullptr);

    font_delete(self->font);
    textures_delete(self->textures);
    shaders_delete(self->shaders);
    window_delete(self->window);

    free(self);
}

void rudyscung_run(rudyscung_t* const self) {
    assert(self != nullptr);

#define LEVEL_SIZE 4
#define LEVEL_HEIGHT 4
    level_t* level = level_new(LEVEL_SIZE, LEVEL_HEIGHT, LEVEL_SIZE);
    for (size_t x = 0; x < LEVEL_SIZE * CHUNK_SIZE; x++) {
        for (size_t y = 0; y < LEVEL_HEIGHT * CHUNK_SIZE; y++) {
            for (size_t z = 0; z < LEVEL_SIZE * CHUNK_SIZE; z++) {
                if (y < 48) {
                    tile_id_t tile = TILE_ID__STONE;
                    if (y == 47) {
                        tile = TILE_ID__GRASS;
                    }
                    float random = (float) rand() / RAND_MAX;
                    if (random > 0.5f) {
                        tile = TILE_ID__AIR;
                    }
                    level_set_tile(level, x, y, z, tile_get(tile));
                }
            }
        }
    }

    renderer_t* renderer = renderer_new(self);
    renderer_set_level(renderer, level);

    camera_t* camera = camera_new(-32.0f, 70.0f, -100.0f);

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
        font_draw(self->font, fps_buffer, 0, 0);

        window_swap(self->window);

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

    camera_delete(camera);
    renderer_delete(renderer);
    level_delete(level);
}

window_t* const rudyscung_get_window(rudyscung_t* const self) {
    assert(self != nullptr);

    return self->window;
}

shaders_t* const rudyscung_get_shaders(rudyscung_t* const self) {
    assert(self != nullptr);

    return self->shaders;
}

textures_t* const rudyscung_get_textures(rudyscung_t* const self) {
    assert(self != nullptr);

    return self->textures;
}

font_t* const rudyscung_get_font(rudyscung_t* const self) {
    assert(self != nullptr);

    return self->font;
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
        pos_dx++;
    }
    if (keys.d) {
        pos_dx--;
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