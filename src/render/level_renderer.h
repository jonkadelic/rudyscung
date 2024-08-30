#pragma once

#include "src/world/level.h"
#include "src/render/camera.h"
#include "src/world/side.h"

typedef struct client client_t;

typedef struct level_renderer level_renderer_t;

typedef struct level_slice {
    size_chunks_t pos[NUM_AXES];
    size_chunks_t size[NUM_AXES];
} level_slice_t;

level_renderer_t* const level_renderer_new(client_t* const client, level_t* const level);

void level_renderer_delete(level_renderer_t* const self);

void level_renderer_slice(level_renderer_t* const self, level_slice_t const* const slice);

void level_renderer_tick(level_renderer_t* const self);

void level_renderer_draw(level_renderer_t* const self, camera_t* const camera, float const partial_tick);

bool const level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const pos[NUM_AXES], side_t const side);
