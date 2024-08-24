#pragma once

#include <stddef.h>

// Predefines
typedef struct rudyscung rudyscung_t;
typedef struct level level_t;
typedef struct camera camera_t;
typedef enum side side_t;

typedef struct level_renderer level_renderer_t;

level_renderer_t* const level_renderer_new(rudyscung_t* const rudyscung, level_t const* const level);

void level_renderer_delete(level_renderer_t* const self);

void level_renderer_tick(level_renderer_t* const self);

void level_renderer_draw(level_renderer_t const* const self, camera_t const* const camera);

bool level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const x, size_t const y, size_t const z, side_t const side);