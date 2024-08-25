#include "./rudyscung.h"

#include <SDL2/SDL_timer.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_video.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_events.h>
#include <SDL_timer.h>

#include "util.h"
#include "render/font.h"
#include "render/level_renderer.h"
#include "render/shaders.h"
#include "render/textures.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "window.h"
#include "world/chunk.h"
#include "world/entity/ecs.h"
#include "world/entity/ecs_components.h"
#include "world/level.h"
#include "world/side.h"

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

static void tick(rudyscung_t* const self, level_t* const level);
static void update_slice(rudyscung_t* const self, level_t* const level, bool const force);

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

#define LEVEL_SIZE 16
#define LEVEL_HEIGHT 8
    level_t* level = level_new(LEVEL_SIZE, LEVEL_HEIGHT, LEVEL_SIZE);
    ecs_t* ecs = level_get_ecs(level);
    entity_t player = level_get_player(level);

    renderer_set_level(self->renderer, level);

    camera_t* camera = camera_new((float[NUM_AXES]) { 0.0f, 0.0f, 0.0f }, (float[NUM_ROT_AXES]) { 0.0f, 0.0f} );

    update_slice(self, level, true);

    bool running = true;
    uint64_t last_game_tick = SDL_GetTicks64();
    while (running) {
        uint64_t const current_tick = SDL_GetTicks64();
        uint64_t const delta_tick = current_tick - last_game_tick;
        float const partial_tick = (delta_tick / (float) MS_PER_TICK) - floor((delta_tick / (float) MS_PER_TICK));

        ecs_component_pos_t* player_pos = ecs_get_component_data(ecs, player, ECS_COMPONENT__POS);
        ecs_component_rot_t* player_rot = ecs_get_component_data(ecs, player, ECS_COMPONENT__ROT);
        if (player_rot->x_rot > M_PI / 2) {
            player_rot->x_rot = M_PI / 2;
        }
        if (player_rot->x_rot < -M_PI / 2) {
            player_rot->x_rot = -M_PI / 2;
        }

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window_handle_resize(self->window);
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    SDL_SetRelativeMouseMode(true);
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                if (SDL_GetRelativeMouseMode()) {
                    player_rot->y_rot += event.motion.xrel / 125.0f;
                    player_rot->x_rot += event.motion.yrel / 125.0f;
                }
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool is_pressed = event.type == SDL_KEYDOWN;
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        SDL_SetRelativeMouseMode(false);
                        break;
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
                    case SDLK_r:
                        if (is_pressed && event.key.repeat == SDL_FALSE) {
                            level_delete(level);
                            level = level_new(LEVEL_SIZE, LEVEL_HEIGHT, LEVEL_SIZE);
                            ecs = level_get_ecs(level);
                            player = level_get_player(level);
                            renderer_set_level(self->renderer, level);
                            update_slice(self, level, true);
                        }
                        break;
                }
            }
        }
        
        renderer_render(self->renderer, camera);

        // Game tick
        size_t ticks = (size_t)(delta_tick / MS_PER_TICK);
        for (uint64_t t = 0; t < ticks; t++) {
            tick(self, level);
        }
        last_game_tick = current_tick - (delta_tick % MS_PER_TICK);

        camera_set_pos(camera, (float[NUM_AXES]) { lerp(player_pos->ox, player_pos->x, partial_tick), lerp(player_pos->oy, player_pos->y, partial_tick), lerp(player_pos->oz, player_pos->z, partial_tick) });
        camera_set_rot(camera, (float[NUM_ROT_AXES]) { player_rot->y_rot, player_rot->x_rot });
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

static void tick(rudyscung_t* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    renderer_tick(self->renderer);
    level_tick(level);

    ecs_t* ecs = level_get_ecs(level);
    entity_t player = level_get_player(level);

    bool any_movement_input = keys.w || keys.s || keys.a || keys.d;
    bool any_vertical_movement_input = keys.shift || keys.space;
    bool any_look_input = keys.left || keys.right || keys.up || keys.down;

    if (any_movement_input) {
        float left = 0;
        float forward = 0;

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

        ecs_component_vel_t* player_vel = ecs_get_component_data(ecs, player, ECS_COMPONENT__VEL);
        ecs_component_rot_t* player_rot = ecs_get_component_data(ecs, player, ECS_COMPONENT__ROT);

        float cos_rot_dy = cos(player_rot->y_rot);
        float sin_rot_dy = sin(player_rot->y_rot);

        float vel_x = - left * cos_rot_dy + forward * sin_rot_dy;
        float vel_z = - left * sin_rot_dy - forward * cos_rot_dy;

        player_vel->vx = vel_x;
        player_vel->vz = vel_z;
    }

    if (any_vertical_movement_input) {
        float up = 0;

        if (keys.space) {
            up++;
        }
        if (keys.shift) {
            up--;
        }

        ecs_component_vel_t* player_vel = ecs_get_component_data(ecs, player, ECS_COMPONENT__VEL);

        float vel_y = up;

        player_vel->vy = vel_y;
    }

    if (any_look_input) {
        float rot_dy = 0;
        float rot_dx = 0;

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

        ecs_component_rot_t* player_rot = ecs_get_component_data(ecs, player, ECS_COMPONENT__ROT);

        float new_rot_y = player_rot->y_rot + rot_dy;
        float new_rot_x = player_rot->x_rot + rot_dx;

        player_rot->y_rot = new_rot_y;
        player_rot->x_rot = new_rot_x;
    }

    update_slice(self, level, false);
}

static void update_slice(rudyscung_t* const self, level_t* const level, bool const force) {
    assert(self != nullptr);

    static int last_chunk_x, last_chunk_y, last_chunk_z;

    ecs_t* ecs = level_get_ecs(level);
    entity_t player = level_get_player(level);

    ecs_component_pos_t* player_pos = ecs_get_component_data(ecs, player, ECS_COMPONENT__POS);

    size_t const slice_diameter = 13;
    size_t const slice_radius = (slice_diameter - 1) / 2;

    size_chunks_t level_size[NUM_AXES];
    level_get_size(level, level_size);

    int const x_camera = floor(player_pos->x / CHUNK_SIZE);
    int const y_camera = floor(player_pos->y / CHUNK_SIZE);
    int const z_camera = floor(player_pos->z / CHUNK_SIZE);

    if (x_camera == last_chunk_x && y_camera == last_chunk_y && z_camera == last_chunk_z && !force) {
        return;
    }

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
        .size = { slice_diameter, 8, slice_radius },
        .pos = { slice_x, 0, slice_z }
    };
    level_renderer_t* const level_renderer = renderer_get_level_renderer(self->renderer);
    level_renderer_slice(level_renderer, &level_slice);

    last_chunk_x = x_camera;
    last_chunk_y = y_camera;
    last_chunk_z = z_camera;
}