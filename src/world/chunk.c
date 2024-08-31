#include "./chunk.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "src/util/logger.h"
#include "src/util/object_counter.h"

#define COORD(pos) (((pos[AXIS__Y]) * CHUNK_SIZE * CHUNK_SIZE) + ((pos[AXIS__Z]) * CHUNK_SIZE) + (pos[AXIS__X]))

struct chunk {
    size_chunks_t pos[NUM_AXES];
    uint8_t tiles[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
    uint8_t tile_shapes[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
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

    OBJ_CTR_INC(chunk_t);

    return self;
}

void chunk_delete(chunk_t* const chunk) {
    assert(chunk != nullptr);

    free(chunk);

    OBJ_CTR_DEC(chunk_t);
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

/* SERIALIZATION FORMAT:
 *     1 byte marker
 *     4 bytes data size (not including preamble)
 *     n bytes data
 */

#define FORMAT_VERSION 1

typedef enum ser_marker {
    SER_MARKER__VERSION,
    SER_MARKER__POS,
    SER_MARKER__TILES,
    SER_MARKER__TILE_SHAPES,
    NUM_SER_MARKERS
} ser_marker_t;

static size_t const EXPECTED_DATA_SIZES[NUM_SER_MARKERS] = {
    [SER_MARKER__VERSION] = 4,
    [SER_MARKER__POS] = 8 + 8 + 8,
    [SER_MARKER__TILES] = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE,
    [SER_MARKER__TILE_SHAPES] = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE
};

size_t const chunk_serialize(chunk_t const* const self, uint8_t* const data) {
    assert(self != nullptr);

    size_t expected_size = 0;
    for (ser_marker_t marker = 0; marker < NUM_SER_MARKERS; marker++) {
        expected_size += 1;
        expected_size += 4;
        expected_size += EXPECTED_DATA_SIZES[marker];
    }

    if (data == nullptr) {
        return expected_size;
    }

    size_t i = 0;

    // Write version
    data[i] = SER_MARKER__VERSION; i += 1;
    *(uint32_t*)(&(data[i])) = EXPECTED_DATA_SIZES[SER_MARKER__VERSION]; i += 4;
    *(uint32_t*)(&(data[i])) = FORMAT_VERSION; i += 4;

    // Write pos
    data[i] = SER_MARKER__POS; i += 1;
    *(uint32_t*)(&(data[i])) = EXPECTED_DATA_SIZES[SER_MARKER__POS]; i += 4;
    *(uint64_t*)(&(data[i])) = self->pos[0]; i += 8;
    *(uint64_t*)(&(data[i])) = self->pos[1]; i += 8;
    *(uint64_t*)(&(data[i])) = self->pos[2]; i += 8;

    // Write tiles
    data[i] = SER_MARKER__TILES; i += 1;
    *(uint32_t*)(&(data[i])) = EXPECTED_DATA_SIZES[SER_MARKER__TILES]; i += 4;
    for (size_t j = 0; j < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; j++) {
        data[i] = self->tiles[j];
        i += 1;
    }

    // Write tile shapes
    data[i] = SER_MARKER__TILE_SHAPES; i += 1;
    *(uint32_t*)(&(data[i])) = EXPECTED_DATA_SIZES[SER_MARKER__TILE_SHAPES]; i += 4;
    for (size_t j = 0; j < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; j++) {
        data[i] = self->tile_shapes[j];
        i += 1;
    }

    return expected_size;
}

chunk_t* const chunk_deserialize(size_t const data_size, uint8_t const data[data_size]) {
    size_t expected_size = 0;
    for (ser_marker_t marker = 0; marker < NUM_SER_MARKERS; marker++) {
        expected_size += 1;
        expected_size += 4;
        expected_size += EXPECTED_DATA_SIZES[marker];
    }

    assert(data_size >= expected_size);

    // Check we have all the appropriate markers
    size_t num_markers[NUM_SER_MARKERS];
    size_t i = 0;
    while (i < data_size) {
        ser_marker_t marker = (ser_marker_t) data[i];
        if (marker >= NUM_SER_MARKERS) {
            LOG_ERROR("Invalid marker %zu at byte %zu!", (size_t) marker, i);
            return nullptr;
        }
        num_markers[marker]++;
        i = i + 1 + 4;
        i += *(uint32_t*)(&(data[i]));
    }
    for (size_t j = 0; j < NUM_SER_MARKERS; j++) {
        if (num_markers[j] != 1) {
            LOG_ERROR("%zu instances of marker found!", num_markers[j]);
            return nullptr;
        }
    }

    chunk_t* chunk = chunk_new((size_chunks_t[NUM_AXES]) { 0, 0, 0 });

    // Read from markers
    i = 0;
    while (i < data_size) {
        ser_marker_t marker = (ser_marker_t) data[i]; i++;
        size_t data_size = *(uint32_t*)(&(data[i])); i += 4;
        if (data_size < EXPECTED_DATA_SIZES[marker]) {
            LOG_ERROR("Data size for marker is too small - expected %zu, got %zu!", EXPECTED_DATA_SIZES[marker], data_size);
        }
        size_t j = i;
        switch (marker) {
            case SER_MARKER__POS: {
                chunk->pos[AXIS__X] = *(uint64_t*)(&(data[j])); j += 8;
                chunk->pos[AXIS__Y] = *(uint64_t*)(&(data[j])); j += 8;
                chunk->pos[AXIS__Z] = *(uint64_t*)(&(data[j])); j += 8;
                break;
            }
            case SER_MARKER__TILES: {
                memcpy(chunk->tiles, &(data[j]), CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
                break;
            }
            case SER_MARKER__TILE_SHAPES: {
                memcpy(chunk->tiles, &(data[j]), CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
                break;
            }
        }
        i += *(uint32_t*)(&(data[i]));
    }

    return chunk;
}