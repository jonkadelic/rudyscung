#pragma once

#include "./tile.h"
#include "./tile_shape.h"
#include "side.h"

#define CHUNK_SIZE 16

typedef size_t size_chunks_t;

typedef struct chunk chunk_t;

chunk_t* const chunk_new(size_chunks_t const pos[NUM_AXES]);

void chunk_delete(chunk_t* const self);

void chunk_get_pos(chunk_t const* const self, size_chunks_t pos[NUM_AXES]);

tile_t const* const chunk_get_tile(chunk_t const* const self, size_t const pos[NUM_AXES]);

void chunk_set_tile(chunk_t* const self, size_t const pos[NUM_AXES], tile_t const* const tile);

tile_shape_t const chunk_get_tile_shape(chunk_t const* const self, size_t const pos[NUM_AXES]);

void chunk_set_tile_shape(chunk_t* const self, size_t const pos[NUM_AXES], tile_shape_t const shape);
