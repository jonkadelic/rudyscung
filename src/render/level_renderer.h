#pragma once

#include "../world/level.h"
#include "./camera.h"

typedef struct rudyscung rudyscung_t;

typedef struct level_renderer level_renderer_t;

typedef struct level_slice {
    size_chunks_t x;
    size_chunks_t y;
    size_chunks_t z;
    size_chunks_t size_x;
    size_chunks_t size_y;
    size_chunks_t size_z;
} level_slice_t;

level_renderer_t* const level_renderer_new(rudyscung_t* const rudyscung, level_t const* const level);

void level_renderer_delete(level_renderer_t* const self);

void level_renderer_slice(level_renderer_t* const self, level_slice_t const* const slice);

void level_renderer_tick(level_renderer_t* const self);

void level_renderer_draw(level_renderer_t const* const self, camera_t const* const camera);

bool level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const x, size_t const y, size_t const z, side_t const side);