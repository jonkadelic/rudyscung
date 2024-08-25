#include "./level_gen.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "./perlin.h"
#include "../level.h"

struct level_gen {
    perlin_t* perlin;
};

typedef struct entry {
    bool defined;
    bool sides_present[NUM_SIDES];
    bool has_extra;
    struct {
        bool north_west;
        bool south_west;
        bool north_east;
        bool south_east;
    } extra;
} entry_t;

static entry_t const SHAPE_LOOKUP[NUM_TILE_SHAPES] = {
    [TILE_SHAPE__RAMP_NORTH] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = false,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = true
        }
    },
    [TILE_SHAPE__RAMP_SOUTH] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = false,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = true
        }
    },
    [TILE_SHAPE__RAMP_WEST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = false
        }
    },
    [TILE_SHAPE__RAMP_EAST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = false,
            [SIDE__EAST] = true
        }
    },
    [TILE_SHAPE__CORNER_A_NORTH_WEST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = false,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = false
        }
    },
    [TILE_SHAPE__CORNER_A_SOUTH_WEST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = false,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = false
        }
    },
    [TILE_SHAPE__CORNER_A_NORTH_EAST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = false,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = false,
            [SIDE__EAST] = true
        }
    },
    [TILE_SHAPE__CORNER_A_SOUTH_EAST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = false,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = false,
            [SIDE__EAST] = true
        }
    },
    [TILE_SHAPE__CORNER_B_NORTH_WEST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = true
        },
        .has_extra = true,
        .extra = {
            .north_west = true,
            .south_west = true,
            .north_east = true,
            .south_east = false
        }
    },
    [TILE_SHAPE__CORNER_B_SOUTH_WEST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = true
        },
        .has_extra = true,
        .extra = {
            .north_west = true,
            .south_west = true,
            .north_east = false,
            .south_east = true
        }
    },
    [TILE_SHAPE__CORNER_B_NORTH_EAST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = true
        },
        .has_extra = true,
        .extra = {
            .north_west = true,
            .south_west = false,
            .north_east = true,
            .south_east = true
        }
    },
    [TILE_SHAPE__CORNER_B_SOUTH_EAST] = {
        .defined = true,
        .sides_present = {
            [SIDE__NORTH] = true,
            [SIDE__SOUTH] = true,
            [SIDE__BOTTOM] = true,
            [SIDE__TOP] = false,
            [SIDE__WEST] = true,
            [SIDE__EAST] = true
        },
        .has_extra = true,
        .extra = {
            .north_west = false,
            .south_west = true,
            .north_east = true,
            .south_east = true
        }
    }
};

static void look_up_sides(level_t const* const level, size_chunks_t const level_size_tiles[3], size_t const x, size_t const y, size_t const z, bool sides[NUM_SIDES]);

static bool tile_matches_pattern(level_t const* const level, tile_shape_t const tile_shape, bool const sides[NUM_SIDES], size_t const x, size_t const y, size_t const z);

level_gen_t* const level_gen_new(unsigned int seed) {
    level_gen_t* const self = malloc(sizeof(level_gen_t));
    assert(self != nullptr);

    self->perlin = perlin_new(seed);

    return self;
}

void level_gen_delete(level_gen_t* const self) {
    assert(self != nullptr);

    free(self);
}

void level_gen_generate(level_gen_t* const self, chunk_t* const chunk) {
    assert(self != nullptr);
    assert(chunk != nullptr);

    size_chunks_t chunk_pos[3];
    chunk_get_pos(chunk, chunk_pos);

    int seed = 0;

    double grid[CHUNK_SIZE * CHUNK_SIZE];
    float scale = 64.0f;
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            grid[z * CHUNK_SIZE + x] = perlin_get_2d(self->perlin, (chunk_pos[0] * CHUNK_SIZE + x) / scale, (chunk_pos[2] * CHUNK_SIZE + z) / scale);
        }
    }
    double grid2[CHUNK_SIZE * CHUNK_SIZE];
    scale = 16.0f;
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            grid2[z * CHUNK_SIZE + x] = perlin_get_2d(self->perlin, (chunk_pos[0] * CHUNK_SIZE + x) / scale, (chunk_pos[2] * CHUNK_SIZE + z) / scale);
        }
    }
    double grid3[CHUNK_SIZE * CHUNK_SIZE];
    scale = 32.0f;
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            grid3[z * CHUNK_SIZE + x] = perlin_get_2d(self->perlin, (chunk_pos[0] * CHUNK_SIZE + x) / scale, (chunk_pos[2] * CHUNK_SIZE + z) / scale);
        }
    }

    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            size_t wx = chunk_pos[0] * CHUNK_SIZE + x;
            size_t wz = chunk_pos[2] * CHUNK_SIZE + z;

            double hd = grid[z * CHUNK_SIZE + x];
            hd += grid2[z * CHUNK_SIZE + x] * 0.25f;
            if (grid3[z * CHUNK_SIZE + x] > 0.4f) {
                hd -= grid3[z * CHUNK_SIZE + x];
            }
            int yo = 16 - (int)(hd * 16);
            size_t height = 64 + yo;
            for (size_t y = 0; y < CHUNK_SIZE && (chunk_pos[1] * CHUNK_SIZE + y) < height; y++) {
                size_t wy = chunk_pos[1] * CHUNK_SIZE + y;
                chunk_set_tile(chunk, x, y, z, tile_get(TILE_ID__STONE));
            }
            if (height >= chunk_pos[1] * CHUNK_SIZE && height < chunk_pos[1] * CHUNK_SIZE + CHUNK_SIZE) {
                tile_t const* top_tile = tile_get(TILE_ID__GRASS);
                if (height <= 75) {
                    top_tile = tile_get(TILE_ID__SAND);
                }
                chunk_set_tile(chunk, x, height - (chunk_pos[1] * CHUNK_SIZE), z, top_tile);
            }
        }
    }
}

void level_gen_smooth(level_gen_t const* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    size_chunks_t level_size[3];
    level_get_size(level, level_size);
    size_t level_size_tiles[3];
    for (size_t i = 0; i < 3; i++) {
        level_size_tiles[i] = level_size[i] * CHUNK_SIZE;
    }

    tile_t const* const air_tile = tile_get(TILE_ID__AIR);

    for (size_t z = 0; z < level_size_tiles[2]; z++) {
        for (size_t x = 0; x < level_size_tiles[0]; x++) {
            size_t y = level_size_tiles[1] - 1;
            tile_t const* tile;
            while ((tile = level_get_tile(level, x, y, z)) == air_tile) {
                y--;
            }

            bool sides_present[NUM_SIDES];
            look_up_sides(level, level_size_tiles, x, y, z, sides_present);

            if (sides_present[SIDE__NORTH] + sides_present[SIDE__SOUTH] + sides_present[SIDE__WEST] + sides_present[SIDE__EAST] < 2) {
                level_set_tile(level, x, y, z, air_tile);
                y--;
                tile_t const* top_tile = tile_get(TILE_ID__GRASS);
                if (y <= 75) {
                    top_tile = tile_get(TILE_ID__SAND);
                }
                level_set_tile(level, x, y, z, top_tile);
            }

            for (tile_shape_t i = 0; i < NUM_TILE_SHAPES; i++) {
                if (SHAPE_LOOKUP[i].defined) {
                    if (tile_matches_pattern(level, i, sides_present, x, y, z)) {
                        level_set_tile_shape(level, x, y, z, i);
                        if (i == TILE_SHAPE__CORNER_A_NORTH_WEST || i == TILE_SHAPE__CORNER_A_SOUTH_WEST || i == TILE_SHAPE__CORNER_A_NORTH_EAST || i == TILE_SHAPE__CORNER_A_SOUTH_EAST) {
                            tile_t const* top_tile = tile_get(TILE_ID__GRASS);
                            if (y - 1 <= 75) {
                                top_tile = tile_get(TILE_ID__SAND);
                            }
                            level_set_tile(level, x, y - 1, z, top_tile);

                            tile_shape_t below_shape_candidate = i + 4;
                            if (SHAPE_LOOKUP[below_shape_candidate].defined) {
                                bool below_sides_present[NUM_SIDES];
                                look_up_sides(level, level_size_tiles, x, y - 1, z, below_sides_present);
                                below_sides_present[SIDE__TOP] = false;
                                if (tile_matches_pattern(level, below_shape_candidate, below_sides_present, x, y - 1, z) ||
                                    (!below_sides_present[SIDE__NORTH] || !below_sides_present[SIDE__SOUTH] || !below_sides_present[SIDE__WEST] || !below_sides_present[SIDE__EAST])
                                ) {
                                    level_set_tile_shape(level, x, y - 1, z, below_shape_candidate);
                                }
                            }
                        }

                        break;
                    }
                }
            }
        }
    }
}

static void look_up_sides(level_t const* const level, size_chunks_t const level_size_tiles[3], size_t const x, size_t const y, size_t const z, bool sides[NUM_SIDES]) {
    assert(level != nullptr);

    tile_t const* const air_tile = tile_get(TILE_ID__AIR);

    for (side_t i = 0; i < NUM_SIDES; i++) {
        int offsets[3];
        side_get_offsets(i, offsets);
        int tx = x + offsets[0];
        int ty = y + offsets[1];
        int tz = z + offsets[2];
        if (tx < 0 || tx >= level_size_tiles[0] ||
            ty < 0 || ty >= level_size_tiles[1] ||
            tz < 0 || tz >= level_size_tiles[2]
        ) {
            sides[i] = false;
        } else {
            tile_t const* const offset_tile = level_get_tile(level, tx, ty, tz);
            sides[i] = offset_tile != air_tile;
        }
    }
}

static bool tile_matches_pattern(level_t const* const level, tile_shape_t const tile_shape, bool const sides[NUM_SIDES], size_t const x, size_t const y, size_t const z) {
    assert(level != nullptr);
    assert(tile_shape >= 0 && tile_shape < NUM_TILE_SHAPES);

    tile_t const* const air_tile = tile_get(TILE_ID__AIR);

    if (memcmp(sides, SHAPE_LOOKUP[tile_shape].sides_present, sizeof(bool) * NUM_SIDES) == 0) {
        if (SHAPE_LOOKUP[tile_shape].has_extra) {
            struct {
                bool north_west;
                bool south_west;
                bool north_east;
                bool south_east;
            } extra = {
                .north_west = level_get_tile(level, x - 1, y, z - 1) != air_tile,
                .south_west = level_get_tile(level, x + 1, y, z - 1) != air_tile,
                .north_east = level_get_tile(level, x - 1, y, z + 1) != air_tile,
                .south_east = level_get_tile(level, x + 1, y, z + 1) != air_tile
            };
            if (memcmp(&extra, &(SHAPE_LOOKUP[tile_shape].extra), sizeof(extra)) != 0) {
                return false;
            }
        }

        return true;
    }

    return false;
}
