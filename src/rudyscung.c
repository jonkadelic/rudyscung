#include "./rudyscung.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_events.h>
#include <SDL_timer.h>

#include "render/font.h"
#include "render/level_renderer.h"
#include "render/shaders.h"
#include "render/textures.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "world/chunk.h"
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
    renderer_t* renderer;
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

static void tick(rudyscung_t* const self, level_t* const level, camera_t* const camera);
static void update_slice(rudyscung_t* const self, level_t* const level, camera_t* const camera);

rudyscung_t* const rudyscung_new(char const* const resources_path) {
    assert(resources_path != nullptr);

    rudyscung_t* self = malloc(sizeof(rudyscung_t));
    assert(self != nullptr);

    self->window = window_new(WINDOW_TITLE, WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT);
    self->shaders = shaders_new(resources_path);
    self->textures = textures_new(resources_path);
    self->font = font_new(self, resources_path, "default");
    self->renderer = renderer_new(self);

    return self;
}

void rudyscung_delete(rudyscung_t* const self) {
    assert(self != nullptr);

    renderer_delete(self->renderer);
    font_delete(self->font);
    textures_delete(self->textures);
    shaders_delete(self->shaders);
    window_delete(self->window);

    free(self);
}

void rudyscung_run(rudyscung_t* const self) {
    assert(self != nullptr);

#define LEVEL_SIZE 32
#define LEVEL_HEIGHT 8
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

    renderer_set_level(self->renderer, level);

    camera_t* camera = camera_new(0, 70.0f, 0);

    update_slice(self, level, camera);

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

        renderer_render(self->renderer, camera);
        char line_buffer[64];
        snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "FPS: %zu", fps);
        font_draw(self->font, line_buffer, 0, 0);
        float camera_pos[3];
        camera_get_pos(camera, camera_pos);
        snprintf(line_buffer, sizeof(line_buffer) / sizeof(line_buffer[0]), "x: %.2f y: %.2f z: %.2f", camera_pos[0], camera_pos[1], camera_pos[2]);
        font_draw(self->font, line_buffer, 0, 14);

        window_swap(self->window);

        uint64_t current_tick = SDL_GetTicks64();
        uint64_t delta_tick = current_tick - last_tick;
        size_t ticks = (size_t)(delta_tick / MS_PER_TICK);
        for (uint64_t t = 0; t < ticks; t++) {
            tick(self, level, camera);
            renderer_tick(self->renderer);
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

static void tick(rudyscung_t* const self, level_t* const level, camera_t* const camera) {
    assert(self != nullptr);
    assert(camera != nullptr);

    float left = 0;
    float up = 0;
    float forward = 0;
    float rot_dy = 0;
    float rot_dx = 0;

    if (keys.w) {
        forward++;
    }
    if (keys.s) {
        forward--;
    }
    if (keys.a) {
        left++;
    }
    if (keys.d) {
        left--;
    }
    if (keys.left) {
        rot_dy -= 0.05;
    }
    if (keys.right) {
        rot_dy += 0.05;
    }
    if (keys.up) {
        rot_dx -= 0.05;
    }
    if (keys.down) {
        rot_dx += 0.05;
    }
    if (keys.space) {
        up++;
    }
    if (keys.shift) {
        up--;
    }

    float camera_pos[3];
    camera_get_pos(camera, camera_pos);
    float camera_rot[2];
    camera_get_rot(camera, camera_rot);

    float cos_rot_dy = cos(camera_rot[0]);
    float sin_rot_dy = sin(camera_rot[0]);

    float new_x = camera_pos[0] - left * cos_rot_dy + forward * sin_rot_dy;
    float new_y = camera_pos[1] + up;
    float new_z = camera_pos[2] - left * sin_rot_dy - forward * cos_rot_dy;
    float new_rot_y = camera_rot[0] + rot_dy;
    float new_rot_x = camera_rot[1] + rot_dx;

    size_chunks_t const x_camera = floor(camera_pos[0] / CHUNK_SIZE);
    size_chunks_t const y_camera = floor(camera_pos[1] / CHUNK_SIZE);
    size_chunks_t const z_camera = floor(camera_pos[2] / CHUNK_SIZE);

    camera_set_pos(camera, new_x, new_y, new_z);
    camera_set_rot(camera, new_rot_y, new_rot_x);

    size_chunks_t const new_x_camera = floor(new_x / CHUNK_SIZE);
    size_chunks_t const new_y_camera = floor(new_y / CHUNK_SIZE);
    size_chunks_t const new_z_camera = floor(new_z / CHUNK_SIZE);

    if (x_camera != new_x_camera || y_camera != new_y_camera || z_camera != new_z_camera) {
        update_slice(self, level, camera);
    }
}

static void update_slice(rudyscung_t* const self, level_t* const level, camera_t* const camera) {
    assert(self != nullptr);
    assert(camera != nullptr);

    size_t const slice_diameter = 5;
    size_t const slice_radius = (slice_diameter - 1) / 2;

    float camera_pos[3];
    camera_get_pos(camera, camera_pos);

    size_chunks_t level_size[3];
    level_get_size(level, level_size);

    int const x_camera = floor(camera_pos[0] / CHUNK_SIZE);
    int const y_camera = floor(camera_pos[1] / CHUNK_SIZE);
    int const z_camera = floor(camera_pos[2] / CHUNK_SIZE);

    int slice_x = x_camera - slice_radius;
    int slice_y = y_camera - slice_radius;
    int slice_z = z_camera - slice_radius;
    if (slice_x < 0) {
        slice_x = 0;
    }
    if (slice_x >= level_size[0] - slice_diameter) {
        slice_x = level_size[0] - slice_diameter - 1;
    }
    if (slice_y < 0) {
        slice_y = 0;
    }
    if (slice_y >= level_size[1] - slice_diameter) {
        slice_y = level_size[1] - slice_diameter - 1;
    }
    if (slice_z < 0) {
        slice_z = 0;
    }
    if (slice_z >= level_size[2] - slice_diameter) {
        slice_z = level_size[2] - slice_diameter - 1;
    }

    level_slice_t level_slice = {
        .size_x = slice_diameter,
        .size_y = slice_diameter,
        .size_z = slice_diameter,
        .x = slice_x,
        .y = slice_y,
        .z = slice_z
    };
    level_renderer_t* const level_renderer = renderer_get_level_renderer(self->renderer);
    level_renderer_slice(level_renderer, &level_slice);
}