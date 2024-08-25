#include "./level.h"

#include <assert.h>
#include <stdlib.h>

#include "chunk.h"
#include "tile_shape.h"
#include "gen/level_gen.h"

#define CHUNK_COORD(x, y, z) (((y) * self->size_z * self->size_x) + ((z) * self->size_x) + (x))
#define TO_CHUNK_SPACE(coord) ((coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(coord) ((coord) * CHUNK_SIZE)

struct level {
    size_chunks_t size_x;
    size_chunks_t size_y;
    size_chunks_t size_z;
    chunk_t** chunks;
    level_gen_t* level_gen;
};

level_t* const level_new(size_chunks_t const size_x, size_chunks_t const size_y, size_chunks_t const size_z) {
    assert(size_x > 0);
    assert(size_y > 0);
    assert(size_z > 0);

    level_t* const self = malloc(sizeof(level_t));
    assert(self != nullptr);

    self->size_x = size_x;
    self->size_y = size_y;
    self->size_z = size_z;

    self->level_gen = level_gen_new(0);

    self->chunks = malloc(sizeof(chunk_t*) * size_y * size_z * size_x);
    assert(self->chunks != nullptr);

    for (size_chunks_t x = 0; x < size_x; x++) {
        for (size_chunks_t y = 0; y < size_y; y++) {
            for (size_chunks_t z = 0; z < size_z; z++) {
                self->chunks[CHUNK_COORD(x, y, z)] = chunk_new(x, y, z);
                level_gen_generate(self->level_gen, self->chunks[CHUNK_COORD(x, y, z)]);                
            }
        }
    }

    level_gen_smooth(self->level_gen, self);

    return self;
}

void level_delete(level_t* const self) {
    assert(self != nullptr);

    for (size_chunks_t x = 0; x < self->size_x; x++) {
        for (size_chunks_t y = 0; y < self->size_y; y++) {
            for (size_chunks_t z = 0; z < self->size_z; z++) {
                chunk_delete(self->chunks[CHUNK_COORD(x, y, z)]);
                self->chunks[CHUNK_COORD(x, y, z)] = nullptr;
            }
        }
    }
    free(self->chunks);
    free(self);
}

void level_get_size(level_t const* const self, size_chunks_t size[3]) {
    assert(self != nullptr);

    size[0] = self->size_x;
    size[1] = self->size_y;
    size[2] = self->size_z;
}

chunk_t* const level_get_chunk(level_t const* const self, size_chunks_t const x, size_chunks_t const y, size_chunks_t const z) {
    assert(self != nullptr);
    assert(x >= 0 && x < self->size_x);
    assert(y >= 0 && y < self->size_y);
    assert(z >= 0 && z < self->size_z);

    return self->chunks[CHUNK_COORD(x, y, z)];
}

tile_t const* const level_get_tile(level_t const* const self, size_t const x, size_t const y, size_t const z) {
    assert(self != nullptr);
    assert(x >= 0 && x < TO_TILE_SPACE(self->size_x));
    assert(y >= 0 && y < TO_TILE_SPACE(self->size_y));
    assert(z >= 0 && z < TO_TILE_SPACE(self->size_z));

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE(x), TO_CHUNK_SPACE(y), TO_CHUNK_SPACE(z));

    return chunk_get_tile(chunk, x % CHUNK_SIZE, y % CHUNK_SIZE, z % CHUNK_SIZE);
}

void level_set_tile(level_t* const self, size_t const x, size_t const y, size_t const z, tile_t const* const tile) {
    assert(self != nullptr);
    assert(x >= 0 && x < TO_TILE_SPACE(self->size_x));
    assert(y >= 0 && y < TO_TILE_SPACE(self->size_y));
    assert(z >= 0 && z < TO_TILE_SPACE(self->size_z));

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE(x), TO_CHUNK_SPACE(y), TO_CHUNK_SPACE(z));

    chunk_set_tile(chunk, x % CHUNK_SIZE, y % CHUNK_SIZE, z % CHUNK_SIZE, tile);
}

tile_shape_t const level_get_tile_shape(level_t const* const self, size_t const x, size_t const y, size_t const z) {
    assert(self != nullptr);
    assert(x >= 0 && x < TO_TILE_SPACE(self->size_x));
    assert(y >= 0 && y < TO_TILE_SPACE(self->size_y));
    assert(z >= 0 && z < TO_TILE_SPACE(self->size_z));

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE(x), TO_CHUNK_SPACE(y), TO_CHUNK_SPACE(z));

    return chunk_get_tile_shape(chunk, x % CHUNK_SIZE, y % CHUNK_SIZE, z % CHUNK_SIZE);
}

void level_set_tile_shape(level_t* const self, size_t const x, size_t const y, size_t const z, tile_shape_t const shape) {
    assert(self != nullptr);
    assert(x >= 0 && x < TO_TILE_SPACE(self->size_x));
    assert(y >= 0 && y < TO_TILE_SPACE(self->size_y));
    assert(z >= 0 && z < TO_TILE_SPACE(self->size_z));

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE(x), TO_CHUNK_SPACE(y), TO_CHUNK_SPACE(z));

    chunk_set_tile_shape(chunk, x % CHUNK_SIZE, y % CHUNK_SIZE, z % CHUNK_SIZE, shape);
}
