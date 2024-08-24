#pragma once

#include "../rudyscung.h"
#include "../world/level.h"
#include "./camera.h"

typedef struct renderer renderer_t;

renderer_t* const renderer_new(rudyscung_t* const rudyscung);

void renderer_delete(renderer_t* const self);

void renderer_tick(renderer_t* const self);

void renderer_set_level(renderer_t* const self, level_t const* const level);

void renderer_render(renderer_t* const self, camera_t const* const camera);
