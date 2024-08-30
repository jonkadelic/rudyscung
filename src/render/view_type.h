#pragma once

#include <SDL2/SDL_events.h>

#include "src/render/camera.h"
#include "src/world/entity/ecs.h"
#include "src/world/level.h"

typedef struct rudyscung rudyscung_t;

typedef struct view_type view_type_t;
typedef struct view_type view_type_entity_t;
typedef struct view_type view_type_isometric_t;

view_type_entity_t* const view_type_entity_new(rudyscung_t* const rudyscung, entity_t const entity);

view_type_isometric_t* const view_type_isometric_new(rudyscung_t* const rudyscung, entity_t const player);

void view_type_delete(view_type_t* const self);

camera_t const* const view_type_get_camera(view_type_t const* const self);

bool const view_type_handle_event(view_type_t* const self, SDL_Event const* const event, level_t* const level);

void view_type_tick(view_type_t* const self, level_t* const level);

void view_type_render_tick(view_type_t* const self, level_t* const level, float const partial_tick);
