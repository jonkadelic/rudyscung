#pragma once

#include "./tile.h"
#include "./tile_shape.h"
#include "./side.h"

#define CHUNK_SIZE 16

typedef size_t size_chunks_t;

typedef struct chunk chunk_t;

chunk_t* const chunk_new(size_chunks_t const x, size_chunks_t const y, size_chunks_t const z);

void chunk_delete(chunk_t* const self);

void chunk_get_pos(chunk_t const* const self, size_chunks_t pos[3]);

tile_t const* const chunk_get_tile(chunk_t const* const self, size_t const x, size_t const y, size_t const z);

void chunk_set_tile(chunk_t* const self, size_t const x, size_t const y, size_t const z, tile_t const* const tile);

tile_shape_t const chunk_get_tile_shape(chunk_t const* const self, size_t const x, size_t const y, size_t const z);

void chunk_set_tile_shape(chunk_t* const self, size_t const x, size_t const y, size_t const z, tile_shape_t const shape);

bool chunk_is_tile_side_occluded(chunk_t const* const self, size_t const x, size_t const y, size_t const z, side_t const side);