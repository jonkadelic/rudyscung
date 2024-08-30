#include "./client.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>

#include "src/render/font.h"
#include "src/render/level_renderer.h"
#include "src/render/shaders.h"
#include "src/render/textures.h"
#include "src/render/camera.h"
#include "src/render/renderer.h"
#include "src/client/window.h"
#include "src/world/chunk.h"
#include "src/world/entity/ecs.h"
#include "src/world/level.h"
#include "src/world/side.h"
#include "src/render/view_type.h"
#include "src/util/logger.h"

#define WINDOW_TITLE "rudyscung"
#define WINDOW_INITIAL_WIDTH 800
#define WINDOW_INITIAL_HEIGHT 600

#define TICKS_PER_SECOND 20
#define MS_PER_TICK (1000 / (TICKS_PER_SECOND))

struct client {
    window_t* window;
    shaders_t* shaders;
    textures_t* textures;
    font_t* font;
    renderer_t* renderer;
    view_type_t* view_type;
};

static void tick(client_t* const self, level_t* const level);
static void update_slice(client_t* const self, level_t* const level, bool const force);

client_t* const client_new(char const* const resources_path) {
    assert(resources_path != nullptr);

    client_t* self = malloc(sizeof(client_t));
    assert(self != nullptr);

    self->window = window_new(WINDOW_TITLE, WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT);
    self->shaders = shaders_new(resources_path);
    self->textures = textures_new(resources_path);
    self->font = font_new(self, resources_path, FONT_NAME__DEFAULT);
    self->renderer = renderer_new(self);
    
    LOG_DEBUG("client_t: initialized.");

    return self;
}

void client_delete(client_t* const self) {
    assert(self != nullptr);

    view_type_delete(self->view_type);
    renderer_delete(self->renderer);
    font_delete(self->font);
    textures_delete(self->textures);
    shaders_delete(self->shaders);
    window_delete(self->window);

    free(self);

    LOG_DEBUG("client_t: deleted.");
}

void client_run(client_t* const self) {
    assert(self != nullptr);

    LOG_INFO("client_t: starting core game loop...");

#define LEVEL_SIZE 16
#define LEVEL_HEIGHT 8
    level_t* level = level_new((size_chunks_t[NUM_AXES]) { LEVEL_SIZE, LEVEL_HEIGHT, LEVEL_SIZE });
    entity_t player = level_get_player(level);

    renderer_set_level(self->renderer, level);

    self->view_type = view_type_isometric_new(self, player);

    update_slice(self, level, true);

    bool running = true;
    uint64_t last_game_tick = SDL_GetTicks64();
    while (running) {
        uint64_t const current_tick = SDL_GetTicks64();
        uint64_t const delta_tick = current_tick - last_game_tick;
        float const partial_tick = (delta_tick / (float) MS_PER_TICK) - floor((delta_tick / (float) MS_PER_TICK));

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!view_type_handle_event(self->view_type, &event, level)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                if (event.type == SDL_WINDOWEVENT) {
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        window_handle_resize(self->window);
                    }
                }
                if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                    bool is_pressed = event.type == SDL_KEYDOWN;
                    switch (event.key.keysym.sym) {
                        case SDLK_r:
                            if (is_pressed && event.key.repeat == SDL_FALSE) {
                                level_delete(level);
                                level = level_new((size_chunks_t[NUM_AXES]) { LEVEL_SIZE, LEVEL_HEIGHT, LEVEL_SIZE });
                                player = level_get_player(level);
                                renderer_set_level(self->renderer, level);
                                view_type_delete(self->view_type);
                                self->view_type = view_type_isometric_new(self, player);
                                update_slice(self, level, true);
                            }
                            break;
                    }
                }
            }
        }
        
        view_type_render_tick(self->view_type, level, partial_tick);
        renderer_render(self->renderer, view_type_get_camera(self->view_type), partial_tick);

        // Game tick
        size_t ticks = (size_t) (delta_tick / MS_PER_TICK);
        for (uint64_t t = 0; t < ticks; t++) {
            view_type_tick(self->view_type, level);
            tick(self, level);
        }
        last_game_tick = current_tick - (delta_tick % MS_PER_TICK);
    }

    level_delete(level);
}

window_t* const client_get_window(client_t* const self) {
    assert(self != nullptr);

    return self->window;
}

shaders_t* const client_get_shaders(client_t* const self) {
    assert(self != nullptr);

    return self->shaders;
}

textures_t* const client_get_textures(client_t* const self) {
    assert(self != nullptr);

    return self->textures;
}

font_t* const client_get_font(client_t* const self) {
    assert(self != nullptr);

    return self->font;
}

void client_set_view_type(client_t* const self, view_type_t* const view_type) {
    assert(self != nullptr);
    assert(view_type != nullptr);

    view_type_delete(self->view_type);
    self->view_type = view_type;
}

static void tick(client_t* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    renderer_tick(self->renderer);
    level_tick(level);

    update_slice(self, level, false);
}

static void update_slice(client_t* const self, level_t* const level, bool const force) {
    assert(self != nullptr);

    static int last_chunk_pos[NUM_AXES];

    camera_t const* const camera = view_type_get_camera(self->view_type);

    float pos[NUM_AXES];
    camera_get_pos(camera, pos);

    size_t const slice_diameter = 13;
    size_t const slice_radius = (slice_diameter - 1) / 2;

    size_chunks_t level_size[NUM_AXES];
    level_get_size(level, level_size);

    int const camera_pos[NUM_AXES] = {
        floor(pos[AXIS__X] / CHUNK_SIZE),
        floor(pos[AXIS__Y] / CHUNK_SIZE),
        floor(pos[AXIS__Z] / CHUNK_SIZE)
    };

    if (memcmp(camera_pos, last_chunk_pos, sizeof(int) * NUM_AXES) == 0 && !force) {
        return;
    }

    int slice_pos[NUM_AXES] = {
        camera_pos[AXIS__X] - slice_radius,
        camera_pos[AXIS__Y] - slice_radius,
        camera_pos[AXIS__Z] - slice_radius
    };
    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (slice_pos[a] < 0) {
            slice_pos[a] = 0;
        }
        if (slice_pos[a] >= level_size[a] - slice_diameter) {
            slice_pos[a] = level_size[a] - slice_diameter;
        }
    }

    level_slice_t level_slice = {
        .size = { slice_diameter, 8, slice_diameter },
        .pos = { slice_pos[AXIS__X], 0, slice_pos[AXIS__Z] }
    };
    level_renderer_t* const level_renderer = renderer_get_level_renderer(self->renderer);
    level_renderer_slice(level_renderer, &level_slice);

    memcpy(last_chunk_pos, camera_pos, sizeof(int) * NUM_AXES);
}