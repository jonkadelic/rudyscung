#pragma once

#include "../world/level.h"
#include "./camera.h"
#include "level_renderer.h"

typedef struct rudyscung rudyscung_t;

typedef struct renderer renderer_t;

renderer_t* const renderer_new(rudyscung_t* const rudyscung);

void renderer_delete(renderer_t* const self);

level_renderer_t* const renderer_get_level_renderer(renderer_t* const self);

void renderer_tick(renderer_t* const self);

void renderer_set_level(renderer_t* const self, level_t* const level);

void renderer_render(renderer_t* const self, camera_t const* const camera);
