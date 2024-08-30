#include "./view_type.h"

#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL.h>

#include "src/render/camera.h"
#include "src/phys/raycast.h"
#include "src/util/util.h"
#include "src/render/font.h"
#include "src/client/client.h"
#include "src/world/entity/ecs.h"
#include "src/world/entity/ecs_components.h"
#include "src/world/level.h"
#include "src/world/side.h"
#include "src/util/logger.h"
#include "src/world/tile.h"
#include "src/phys/aabb.h"

typedef enum view_type_id {
    VIEW_TYPE_ID__ENTITY,
    VIEW_TYPE_ID__ISOMETRIC,
    NUM_VIEW_TYPE_IDS
} view_type_id_t;

typedef bool (*handle_event)(view_type_t* const self, SDL_Event const* const event, level_t* const level);
typedef void (*tick)(view_type_t* const self, level_t* const level);
typedef void (*render_tick)(view_type_t* const self, level_t* const level, float const partial_tick);

typedef struct keys {
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
    bool left_click;
    bool right_click;
} keys_t;

struct view_type {
    view_type_id_t view_type_id;
    client_t* client;
    camera_t* camera;
    handle_event handle_event;
    tick tick;
    render_tick render_tick;
};

typedef struct _view_type_entity {
    view_type_t super;
    entity_t entity;
    keys_t keys;
} _view_type_entity_t;

typedef struct _view_type_isometric {
    view_type_t super;
    entity_t player;
    keys_t keys;
} _view_type_isometric_t;

static view_type_t* const view_type_new(view_type_id_t const view_type_id, client_t* const client, camera_t* const camera, handle_event const handle_event, tick const tick, render_tick const render_tick, size_t const size);

static bool entity_handle_event(_view_type_entity_t* const self, SDL_Event const* const event, level_t* const level);

static bool isometric_handle_event(_view_type_isometric_t* const self, SDL_Event const* const event, level_t* const level);

static void entity_tick(_view_type_entity_t* const self, level_t* const level);

static void isometric_tick(_view_type_isometric_t* const self, level_t* const level);

static void entity_render_tick(_view_type_entity_t* const self, level_t* const level, float const partial_tick);

static void isometric_render_tick(_view_type_isometric_t* const self, level_t* const level, float const partial_tick);

view_type_entity_t* const view_type_entity_new(client_t* const client, entity_t const entity) {
    _view_type_entity_t* const self = (_view_type_entity_t*) view_type_new(VIEW_TYPE_ID__ENTITY, client, camera_perspective_new(), (handle_event) entity_handle_event, (tick) entity_tick, (render_tick) entity_render_tick, sizeof(_view_type_entity_t));

    self->entity = entity;

    memset(&(self->keys), 0, sizeof(keys_t));
    
    return (view_type_entity_t*) self;
}

view_type_isometric_t* const view_type_isometric_new(client_t* const client, entity_t const player) {
    _view_type_isometric_t* const self = (_view_type_isometric_t*) view_type_new(VIEW_TYPE_ID__ISOMETRIC, client, camera_ortho_new(), (handle_event) isometric_handle_event, (tick) isometric_tick, (render_tick) isometric_render_tick, sizeof(_view_type_isometric_t));

    self->player = player;

    memset(&(self->keys), 0, sizeof(keys_t));

    return (view_type_isometric_t*) self;
}

void view_type_delete(view_type_t* const self) {
    assert(self != nullptr);

    camera_delete(self->camera);

    free(self);
}

camera_t const* const view_type_get_camera(view_type_t const* const self) {
    assert(self != nullptr);

    return self->camera;
}

bool const view_type_handle_event(view_type_t* const self, SDL_Event const* const event, level_t* const level) {
    assert(self != nullptr);
    assert(event != nullptr);

    return self->handle_event(self, event, level);
}

void view_type_tick(view_type_t* const self, level_t* const level) {
    assert(self != nullptr);

    self->tick(self, level);
}

void view_type_render_tick(view_type_t* const self, level_t* const level, float const partial_tick) {
    assert(self != nullptr);

    self->render_tick(self, level, partial_tick);
}

static view_type_t* const view_type_new(view_type_id_t const view_type_id, client_t* const client, camera_t* const camera, handle_event const handle_event, tick const tick, render_tick const render_tick, size_t const size) {
    assert(view_type_id >= 0 && view_type_id < NUM_VIEW_TYPE_IDS);
    assert(client != nullptr);
    assert(camera != nullptr);
    assert(handle_event != nullptr);
    assert(tick != nullptr);
    assert(render_tick != nullptr);
    assert(size > 0);

    view_type_t* const self = malloc(size);
    assert(self != nullptr);

    self->view_type_id = view_type_id;
    self->client = client;
    self->camera = camera;
    self->handle_event = handle_event;
    self->tick = tick;
    self->render_tick = render_tick;

    return self;
}

static bool entity_handle_event(_view_type_entity_t* const self, SDL_Event const* const event, level_t* const level) {
    assert(self != nullptr);
    assert(event != nullptr);
    assert(level != nullptr);

    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    if (SDL_GetRelativeMouseMode()) {
                        self->keys.left_click = true;
                        return true;
                    } else {
                        SDL_SetRelativeMouseMode(true);
                        return true;
                    }
                    break;
                }
                case SDL_BUTTON_RIGHT: {
                    if (SDL_GetRelativeMouseMode()) {
                        self->keys.right_click = true;
                        return true;
                    }
                    break;
                }
            }
            return false;
        }
        case SDL_MOUSEBUTTONUP: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    self->keys.left_click = false;
                    return true;
                }
                case SDL_BUTTON_RIGHT: {
                    self->keys.right_click = false;
                    return true;
                }
            }
            return false;
        }
        case SDL_MOUSEMOTION: {
            if (SDL_GetRelativeMouseMode()) {
                ecs_t* const ecs = level_get_ecs(level);
                if (ecs_has_component(ecs, self->entity, ECS_COMPONENT__ROT)) {
                    ecs_component_rot_t* const entity_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);

                    entity_rot->rot[ROT_AXIS__Y] += event->motion.xrel / 125.0f;
                    entity_rot->rot[ROT_AXIS__X] += event->motion.yrel / 125.0f;
                    return true;
                }
            }
            return false;
        }
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            bool is_pressed = event->type == SDL_KEYDOWN;
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE:
                    ecs_t* const ecs = level_get_ecs(level);
                    ecs_detach_component(ecs, self->entity, ECS_COMPONENT__CONTROLLED);
                    view_type_isometric_t* const view_type = view_type_isometric_new(self->super.client, level_get_player(level));
                    client_set_view_type(self->super.client, view_type);
                    return true;
                case SDLK_w:
                    self->keys.w = is_pressed;
                    return true;
                case SDLK_s:
                    self->keys.s = is_pressed;
                    return true;
                case SDLK_a:
                    self->keys.a = is_pressed;
                    return true;
                case SDLK_d:
                    self->keys.d = is_pressed;
                    return true;
                case SDLK_LEFT:
                    self->keys.left = is_pressed;
                    return true;
                case SDLK_RIGHT:
                    self->keys.right = is_pressed;
                    return true;
                case SDLK_UP:
                    self->keys.up = is_pressed;
                    return true;
                case SDLK_DOWN:
                    self->keys.down = is_pressed;
                    return true;
                case SDLK_SPACE:
                    self->keys.space = is_pressed;
                    return true;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    self->keys.shift = is_pressed;
                    return true;
            }
            return false;
        }
    }

    return false;
}

static bool isometric_handle_event(_view_type_isometric_t* const self, SDL_Event const* const event, level_t* const level) {
    assert(self != nullptr);
    assert(event != nullptr);
    assert(level != nullptr);

    switch (event->type) {
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
            bool is_pressed = event->type == SDL_MOUSEBUTTONDOWN;
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: {
                    self->keys.left_click = is_pressed;
                    return true;
                }
                case SDL_BUTTON_RIGHT: {
                    self->keys.right_click = is_pressed;
                    return true;
                }
            }
            return false;
        }
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            bool is_pressed = event->type == SDL_KEYDOWN;
            switch (event->key.keysym.sym) {
                case SDLK_w:
                    self->keys.w = is_pressed;
                    return true;
                case SDLK_s:
                    self->keys.s = is_pressed;
                    return true;
                case SDLK_a:
                    self->keys.a = is_pressed;
                    return true;
                case SDLK_d:
                    self->keys.d = is_pressed;
                    return true;
                case SDLK_LEFT:
                    self->keys.left = is_pressed;
                    return true;
                case SDLK_RIGHT:
                    self->keys.right = is_pressed;
                    return true;
                case SDLK_UP:
                    self->keys.up = is_pressed;
                    return true;
                case SDLK_DOWN:
                    self->keys.down = is_pressed;
                    return true;
                case SDLK_SPACE:
                    self->keys.space = is_pressed;
                    return true;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    self->keys.shift = is_pressed;
                    return true;
            }
            return false;
        }
    }

    return false;
}

static void entity_tick(_view_type_entity_t* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    static uint64_t last_block_break = 0;

    ecs_t* ecs = level_get_ecs(level);

    bool const has_pos = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__POS);
    bool const has_vel = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__VEL);
    bool const has_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);

    bool any_movement_input = self->keys.w || self->keys.s || self->keys.a || self->keys.d;
    bool any_vertical_movement_input = self->keys.shift || self->keys.space;
    bool any_look_input = self->keys.left || self->keys.right || self->keys.up || self->keys.down;
    bool any_click = self->keys.left_click || self->keys.right_click;

    if (has_rot) {
        ecs_component_rot_t* entity_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);

        if (entity_rot->rot[ROT_AXIS__X] > M_PI / 2) {
            entity_rot->rot[ROT_AXIS__X] = M_PI / 2;
        }
        if (entity_rot->rot[ROT_AXIS__X] < -M_PI / 2) {
            entity_rot->rot[ROT_AXIS__X] = -M_PI / 2;
        }
    }

    if (any_movement_input && has_vel && has_rot) {
        float left = 0;
        float forward = 0;
        float const speed = 0.5f;

        if (self->keys.w) {
            forward += speed;
        }
        if (self->keys.s) {
            forward -= speed;
        }
        if (self->keys.a) {
            left += speed;
        }
        if (self->keys.d) {
            left -= speed;
        }

        ecs_component_vel_t* entity_vel = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__VEL);
        ecs_component_rot_t* entity_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);

        float cos_rot_dy = cos(entity_rot->rot[ROT_AXIS__Y]);
        float sin_rot_dy = sin(entity_rot->rot[ROT_AXIS__Y]);

        float vel_x = - left * cos_rot_dy + forward * sin_rot_dy;
        float vel_z = - left * sin_rot_dy - forward * cos_rot_dy;

        entity_vel->vel[AXIS__X] = vel_x;
        entity_vel->vel[AXIS__Z] = vel_z;
    }

    if (any_vertical_movement_input && has_vel) {
        float up = 0;
        float const speed = 0.5f;

        if (self->keys.space) {
            up += speed;
        }
        if (self->keys.shift) {
            up -= speed;
        }

        ecs_component_vel_t* entity_vel = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__VEL);

        float vel_y = up;

        entity_vel->vel[AXIS__Y] = vel_y;
    }

    if (any_look_input && has_rot) {
        float rot_dy = 0;
        float rot_dx = 0;
        float const speed = 0.25f;

        if (self->keys.left) {
            rot_dy -= speed;
        }
        if (self->keys.right) {
            rot_dy += speed;
        }
        if (self->keys.up) {
            rot_dx -= speed;
        }
        if (self->keys.down) {
            rot_dx += speed;
        }

        ecs_component_rot_t* entity_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);

        float new_rot_y = entity_rot->rot[ROT_AXIS__Y] + rot_dy;
        float new_rot_x = entity_rot->rot[ROT_AXIS__X] + rot_dx;

        entity_rot->rot[ROT_AXIS__Y] = new_rot_y;
        entity_rot->rot[ROT_AXIS__X] = new_rot_x;
    }

    if (any_click && has_pos && has_rot) {
        ecs_component_pos_t const* const entity_pos = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__POS);
        ecs_component_rot_t const* const entity_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);

        uint64_t current_tick = SDL_GetTicks64();

        if (current_tick - last_block_break > 500) {
            last_block_break = current_tick;

            raycast_t raycast;
            raycast_cast_in_level(&raycast, level, entity_pos->pos, entity_rot->rot);

            if (raycast.hit) {
                if (self->keys.left_click) {
                    level_set_tile(level, raycast.tile_pos, TILE__AIR);
                } else if (self->keys.right_click) {
                    int offset[NUM_AXES];
                    side_get_offsets(raycast.side, offset);
                    level_set_tile(level, VEC_ADD(raycast.tile_pos, offset), TILE__STONE);
                }
            }
        }
    }
}

static void isometric_tick(_view_type_isometric_t* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    static uint64_t last_block_break = 0;

    ecs_t* ecs = level_get_ecs(level);

    ecs_component_vel_t* player_vel = ecs_get_component_data(ecs, self->player, ECS_COMPONENT__VEL);
    ecs_component_rot_t* player_rot = ecs_get_component_data(ecs, self->player, ECS_COMPONENT__ROT);

    if (player_rot->rot[ROT_AXIS__X] > M_PI / 2) {
        player_rot->rot[ROT_AXIS__X] = M_PI / 2;
    }
    if (player_rot->rot[ROT_AXIS__X] < -M_PI / 2) {
        player_rot->rot[ROT_AXIS__X] = -M_PI / 2;
    }

    bool any_movement_input = self->keys.w || self->keys.s || self->keys.a || self->keys.d;
    bool any_vertical_movement_input = self->keys.up || self->keys.down;
    bool any_look_input = self->keys.left || self->keys.right;

    if (any_movement_input) {
        float left = 0;
        float forward = 0;
        float const speed = 2.0f;

        if (self->keys.w) {
            forward += speed;
        }
        if (self->keys.s) {
            forward -= speed;
        }
        if (self->keys.a) {
            left += speed;
        }
        if (self->keys.d) {
            left -= speed;
        }

        float cos_rot_dy = cos(player_rot->rot[ROT_AXIS__Y]);
        float sin_rot_dy = sin(player_rot->rot[ROT_AXIS__Y]);

        float vel_x = - left * cos_rot_dy + forward * sin_rot_dy;
        float vel_z = - left * sin_rot_dy - forward * cos_rot_dy;

        player_vel->vel[AXIS__X] = vel_x;
        player_vel->vel[AXIS__Z] = vel_z;
    }

    if (any_vertical_movement_input) {
        float up = 0;
        float const speed = 0.125f;

        if (self->keys.up) {
            up -= speed;
        }
        if (self->keys.down) {
            up += speed;
        }

        float vel_y = up;

        player_vel->vel[AXIS__Y] = vel_y;
    }

    if (any_look_input) {
        float rot_dy = 0;
        float const speed = 0.25f;

        if (self->keys.left) {
            rot_dy -= speed;
        }
        if (self->keys.right) {
            rot_dy += speed;
        }

        float new_rot_y = player_rot->rot[ROT_AXIS__Y] + rot_dy;

        player_rot->rot[ROT_AXIS__Y] = new_rot_y;
    }

    if (self->keys.left_click) {

        uint64_t current_tick = SDL_GetTicks64();

        if (current_tick - last_block_break > 500) {
            last_block_break = current_tick;

            int mouse_x, mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);

            window_t const* const window = client_get_window(self->super.client);
            size_t window_size[2];
            window_get_size(window, window_size);

            mouse_y = window_size[1] - mouse_y;

            float world_pos[NUM_AXES];
            if (camera_pick(self->super.camera, (size_t[2]) { window_size[0], window_size[1] }, (size_t[2]) { mouse_x, mouse_y }, world_pos)) {
                ecs_t* const ecs = level_get_ecs(level);
                entity_t highest_entity = ecs_get_highest_entity_id(ecs);
                aabb_t* aabb = aabb_new_default();
                for (entity_t entity = 0; entity <= highest_entity; entity++) {
                    if (ecs_has_component(ecs, entity, ECS_COMPONENT__POS) && ecs_has_component(ecs, entity, ECS_COMPONENT__AABB)) {
                        ecs_component_pos_t const* const entity_pos = ecs_get_component_data(ecs, entity, ECS_COMPONENT__POS);
                        ecs_component_aabb_t const* const entity_aabb = ecs_get_component_data(ecs, entity, ECS_COMPONENT__AABB);
                        aabb_translate(entity_aabb->aabb, entity_pos->pos, aabb);

                        if (aabb_test_pos_inside(aabb, world_pos)) {
                            LOG_DEBUG("view_type_t: picked entity %zu.", entity);
                            ecs_attach_component(ecs, entity, ECS_COMPONENT__CONTROLLED);
                            view_type_entity_t* view_type = view_type_entity_new(self->super.client, entity);
                            client_set_view_type(self->super.client, view_type);
                            aabb_delete(aabb);
                            return; // Exit as quickly as possible as we're technically operating in an object that no longer exists
                        }
                    }
                }
                aabb_delete(aabb);
            }
        }
    }
}

static void entity_render_tick(_view_type_entity_t* const self, level_t* const level, float const partial_tick) {
    assert(self != nullptr);
    assert(level != nullptr);

    ecs_t* ecs = level_get_ecs(level);

    if (ecs_has_component(ecs, self->entity, ECS_COMPONENT__POS)) {
        ecs_component_pos_t const* const entity_pos = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__POS);
        camera_set_pos(self->super.camera, (float[NUM_AXES]) { lerp(entity_pos->pos_o[AXIS__X], entity_pos->pos[AXIS__X], partial_tick), lerp(entity_pos->pos_o[AXIS__Y] + 1.8f, entity_pos->pos[AXIS__Y] + 1.8f, partial_tick), lerp(entity_pos->pos_o[AXIS__Z], entity_pos->pos[AXIS__Z], partial_tick) });
    }
    if (ecs_has_component(ecs, self->entity, ECS_COMPONENT__ROT)) {
        ecs_component_rot_t const* const entity_rot = ecs_get_component_data(ecs, self->entity, ECS_COMPONENT__ROT);
        camera_set_rot(self->super.camera, entity_rot->rot);
    }
}

static void isometric_render_tick(_view_type_isometric_t* const self, level_t* const level, float const partial_tick) {
    assert(self != nullptr);
    assert(level != nullptr);

    if (SDL_GetRelativeMouseMode()) {
        SDL_SetRelativeMouseMode(false);
    }

    ecs_t* ecs = level_get_ecs(level);

    ecs_component_pos_t const* const player_pos = ecs_get_component_data(ecs, self->player, ECS_COMPONENT__POS);
    ecs_component_rot_t const* const player_rot = ecs_get_component_data(ecs, self->player, ECS_COMPONENT__ROT);

    camera_set_pos(self->super.camera, (float[NUM_AXES]) { lerp(player_pos->pos_o[AXIS__X], player_pos->pos[AXIS__X], partial_tick), lerp(player_pos->pos_o[AXIS__Y], player_pos->pos[AXIS__Y], partial_tick), lerp(player_pos->pos_o[AXIS__Z], player_pos->pos[AXIS__Z], partial_tick) });
    camera_set_rot(self->super.camera, player_rot->rot);
}
