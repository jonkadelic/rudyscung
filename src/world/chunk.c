#include "./chunk.h"
#include "tile.h"
#include "tile_shape.h"

#include <assert.h>
#include <stdlib.h>

#define COORD(x, y, z) (((y) * CHUNK_SIZE * CHUNK_SIZE) + ((z) * CHUNK_SIZE) + (x))

struct chunk {
    int x;
    int y;
    int z;
    tile_id_t tiles[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
    tile_shape_t tile_shapes[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
};

chunk_t* const chunk_new(size_chunks_t const x, size_chunks_t const y, size_chunks_t const z) {
    chunk_t* const self = malloc(sizeof(chunk_t));
    assert(self != nullptr);

    self->x = x;
    self->y = y;
    self->z = z;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                self->tiles[COORD(x, y, z)] = TILE_ID__AIR;
                self->tile_shapes[COORD(x, y, z)] = TILE_SHAPE__NO_RENDER;
            }
        }
    }
    return self;
}

void chunk_delete(chunk_t* const chunk) {
    assert(chunk != nullptr);

    free(chunk);
}

void chunk_get_pos(chunk_t const* const self, size_chunks_t pos[3]) {
    assert(self != nullptr);

    pos[0] = self->x;
    pos[1] = self->y;
    pos[2] = self->z;
}

tile_t const* const chunk_get_tile(chunk_t const* const self, size_t const x, size_t const y, size_t const z) {
    assert(self != nullptr);
    assert(x >= 0 && x < CHUNK_SIZE);
    assert(y >= 0 && y < CHUNK_SIZE);
    assert(z >= 0 && z < CHUNK_SIZE);

    return tile_get(self->tiles[COORD(x, y, z)]);
}

void chunk_set_tile(chunk_t* const self, size_t const x, size_t const y, size_t const z, tile_t const* const tile) {
    assert(self != nullptr);
    assert(x >= 0 && x < CHUNK_SIZE);
    assert(y >= 0 && y < CHUNK_SIZE);
    assert(z >= 0 && z < CHUNK_SIZE);
    assert(tile != nullptr);

    tile_id_t id = tile_get_id(tile);
    self->tiles[COORD(x, y, z)] = id;
    if (id == TILE_ID__AIR) {
        chunk_set_tile_shape(self, x, y, z, TILE_SHAPE__NO_RENDER);
    } else {
        chunk_set_tile_shape(self, x, y, z, TILE_SHAPE__FLAT);
    }
}

tile_shape_t const chunk_get_tile_shape(chunk_t const* const self, size_t const x, size_t const y, size_t const z) {
    assert(self != nullptr);
    assert(x >= 0 && x < CHUNK_SIZE);
    assert(y >= 0 && y < CHUNK_SIZE);
    assert(z >= 0 && z < CHUNK_SIZE);

    return self->tile_shapes[COORD(x, y, z)];
}

void chunk_set_tile_shape(chunk_t* const self, size_t const x, size_t const y, size_t const z, tile_shape_t const shape) {
    assert(self != nullptr);
    assert(x >= 0 && x < CHUNK_SIZE);
    assert(y >= 0 && y < CHUNK_SIZE);
    assert(z >= 0 && z < CHUNK_SIZE);
    assert(shape >= 0 && shape < NUM_TILE_SHAPES);

    self->tile_shapes[COORD(x, y, z)] = shape;
}
