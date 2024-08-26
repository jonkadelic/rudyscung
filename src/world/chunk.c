#include "./chunk.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "side.h"
#include "tile.h"
#include "tile_shape.h"

#define COORD(pos) (((pos[AXIS__Y]) * CHUNK_SIZE * CHUNK_SIZE) + ((pos[AXIS__Z]) * CHUNK_SIZE) + (pos[AXIS__X]))

struct chunk {
    size_chunks_t pos[NUM_AXES];
    tile_t tiles[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
    tile_shape_t tile_shapes[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
};

chunk_t* const chunk_new(size_chunks_t const pos[NUM_AXES]) {
    chunk_t* const self = malloc(sizeof(chunk_t));
    assert(self != nullptr);

    memcpy(self->pos, pos, sizeof(size_chunks_t) * NUM_AXES);
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t y = 0; y < CHUNK_SIZE; y++) {
            for (size_t z = 0; z < CHUNK_SIZE; z++) {
                size_t const i_pos[NUM_AXES] = { x, y, z };
                self->tiles[COORD(i_pos)] = TILE__AIR;
                self->tile_shapes[COORD(i_pos)] = TILE_SHAPE__NO_RENDER;
            }
        }
    }
    return self;
}

void chunk_delete(chunk_t* const chunk) {
    assert(chunk != nullptr);

    free(chunk);
}

void chunk_get_pos(chunk_t const* const self, size_chunks_t pos[NUM_AXES]) {
    assert(self != nullptr);

    memcpy(pos, self->pos, sizeof(size_chunks_t) * NUM_AXES);
}

tile_t const chunk_get_tile(chunk_t const* const self, size_t const pos[NUM_AXES]) {
    assert(self != nullptr); 
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < CHUNK_SIZE);
    }

    return self->tiles[COORD(pos)];
}

void chunk_set_tile(chunk_t* const self, size_t const pos[NUM_AXES], tile_t const tile) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < CHUNK_SIZE);
    }
    assert(tile >= 0 && tile < NUM_TILES);

    self->tiles[COORD(pos)] = tile;
    if (tile == TILE__AIR) {
        chunk_set_tile_shape(self, pos, TILE_SHAPE__NO_RENDER);
    } else {
        chunk_set_tile_shape(self, pos, TILE_SHAPE__FLAT);
    }
}

tile_shape_t const chunk_get_tile_shape(chunk_t const* const self, size_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < CHUNK_SIZE);
    }

    return self->tile_shapes[COORD(pos)];
}

void chunk_set_tile_shape(chunk_t* const self, size_t const pos[NUM_AXES], tile_shape_t const shape) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < CHUNK_SIZE);
    }
    assert(shape >= 0 && shape < NUM_TILE_SHAPES);

    self->tile_shapes[COORD(pos)] = shape;
}
