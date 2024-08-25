#pragma once

#include <stddef.h>

#include "chunk.h"
#include "entity/ecs.h"
#include "side.h"

typedef struct level level_t;

level_t* const level_new(size_chunks_t const size_x, size_chunks_t const size_y, size_chunks_t const size_z);

void level_delete(level_t* const self);

void level_get_size(level_t const* const self, size_chunks_t size[3]);

chunk_t* const level_get_chunk(level_t const* const self, size_chunks_t const x, size_chunks_t const y, size_chunks_t const z);

tile_t const* const level_get_tile(level_t const* const self, size_t const x, size_t const y, size_t const z);

void level_set_tile(level_t* const self, size_t const x, size_t const y, size_t const z, tile_t const* const tile);

tile_shape_t const level_get_tile_shape(level_t const* const self, size_t const x, size_t const y, size_t const z);

void level_set_tile_shape(level_t* const self, size_t const x, size_t const y, size_t const z, tile_shape_t const shape);

void level_tick(level_t* const self);

ecs_t* const level_get_ecs(level_t* const self);

entity_t const level_get_player(level_t const* const self);

float const level_get_distance_on_axis(level_t const* const self, float const x, float const y, float const z, side_t const side, float const max_range);