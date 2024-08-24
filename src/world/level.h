#pragma once

#include <stddef.h>

#include "chunk.h"

typedef struct level level_t;

level_t* const level_new(size_chunks_t const size_x, size_chunks_t const size_y, size_chunks_t const size_z);

void level_delete(level_t* const self);

void level_get_size(level_t const* const self, size_chunks_t size[3]);

chunk_t* const level_get_chunk(level_t const* const self, size_chunks_t const x, size_chunks_t const y, size_chunks_t const z);

tile_t const* const level_get_tile(level_t const* const self, size_t const x, size_t const y, size_t const z);

void level_set_tile(level_t* const self, size_t const x, size_t const y, size_t const z, tile_t const* const tile);

tile_shape_t const level_get_tile_shape(level_t const* const self, size_t const x, size_t const y, size_t const z);

void level_set_tile_shape(level_t* const self, size_t const x, size_t const y, size_t const z, tile_shape_t const shape);
