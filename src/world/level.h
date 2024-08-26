#pragma once

#include <stddef.h>

#include "chunk.h"
#include "entity/ecs.h"
#include "side.h"

typedef struct level level_t;

level_t* const level_new(size_chunks_t const size[NUM_AXES]);

void level_delete(level_t* const self);

void level_get_size(level_t const* const self, size_chunks_t size[NUM_AXES]);

chunk_t* const level_get_chunk(level_t const* const self, size_chunks_t const pos[NUM_AXES]);

tile_t const* const level_get_tile(level_t const* const self, size_t const pos[NUM_AXES]);

void level_set_tile(level_t* const self, size_t const pos[NUM_AXES], tile_t const* const tile);

tile_shape_t const level_get_tile_shape(level_t const* const self, size_t const pos[NUM_AXES]);

void level_set_tile_shape(level_t* const self, size_t const pos[NUM_AXES], tile_shape_t const shape);

void level_tick(level_t* const self);

ecs_t* const level_get_ecs(level_t* const self);

entity_t const level_get_player(level_t const* const self);

float const level_get_nearest_face_on_axis(level_t const* const self, float const pos[NUM_AXES], side_t const side, float const max_range);