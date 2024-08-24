#pragma once

#include "../world/level.h"
#include "./textures.h"
#include "./camera.h"

typedef struct level_renderer level_renderer_t;

level_renderer_t* const level_renderer_new(textures_t* const textures, level_t const* const level);

void level_renderer_delete(level_renderer_t* const self);

void level_renderer_tick(level_renderer_t* const self);

void level_renderer_draw(level_renderer_t const* const self, camera_t const* const camera);