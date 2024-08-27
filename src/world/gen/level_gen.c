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

static void look_up_sides(level_t const* const level, size_chunks_t const level_size_tiles[NUM_AXES], size_t const pos[NUM_AXES], bool sides[NUM_SIDES]);

static bool tile_matches_pattern(level_t const* const level, tile_shape_t const tile_shape, bool const sides[NUM_SIDES], size_t const pos[NUM_AXES]);

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

    size_chunks_t chunk_pos[NUM_AXES];
    chunk_get_pos(chunk, chunk_pos);

    int seed = 0;

    double grid[CHUNK_SIZE * CHUNK_SIZE];
    float scale = 64.0f;
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            grid[z * CHUNK_SIZE + x] = perlin_get_2d(self->perlin, (chunk_pos[AXIS__X] * CHUNK_SIZE + x) / scale, (chunk_pos[AXIS__Z] * CHUNK_SIZE + z) / scale);
        }
    }
    double grid2[CHUNK_SIZE * CHUNK_SIZE];
    scale = 16.0f;
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            grid2[z * CHUNK_SIZE + x] = perlin_get_2d(self->perlin, (chunk_pos[AXIS__X] * CHUNK_SIZE + x) / scale, (chunk_pos[AXIS__Z] * CHUNK_SIZE + z) / scale);
        }
    }
    double grid3[CHUNK_SIZE * CHUNK_SIZE];
    scale = 32.0f;
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            grid3[z * CHUNK_SIZE + x] = perlin_get_2d(self->perlin, (chunk_pos[AXIS__X] * CHUNK_SIZE + x) / scale, (chunk_pos[AXIS__Z] * CHUNK_SIZE + z) / scale);
        }
    }

    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t z = 0; z < CHUNK_SIZE; z++) {
            size_t wx = chunk_pos[AXIS__X] * CHUNK_SIZE + x;
            size_t wz = chunk_pos[AXIS__Z] * CHUNK_SIZE + z;

            double hd = grid[z * CHUNK_SIZE + x];
            hd += grid2[z * CHUNK_SIZE + x] * 0.25f;
            if (grid3[z * CHUNK_SIZE + x] > 0.4f) {
                hd -= grid3[z * CHUNK_SIZE + x];
            }
            int yo = 16 - (int)(hd * 16);
            size_t height = 64 + yo;
            for (size_t y = 0; y < CHUNK_SIZE && (chunk_pos[AXIS__Y] * CHUNK_SIZE + y) < height; y++) {
                size_t wy = chunk_pos[AXIS__Y] * CHUNK_SIZE + y;
                chunk_set_tile(chunk, (size_t[NUM_AXES]) { x, y, z }, TILE__STONE);
            }
            if (height >= chunk_pos[AXIS__Y] * CHUNK_SIZE && height < chunk_pos[AXIS__Y] * CHUNK_SIZE + CHUNK_SIZE) {
                tile_t top_tile = TILE__GRASS;
                if (height <= 75) {
                    top_tile = TILE__SAND;
                }
                chunk_set_tile(chunk, (size_t[NUM_AXES]) { x, height - (chunk_pos[AXIS__Y] * CHUNK_SIZE), z }, top_tile);
            }
        }
    }
}

void level_gen_smooth(level_gen_t const* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    size_chunks_t level_size[NUM_AXES];
    level_get_size(level, level_size);
    size_t level_size_tiles[NUM_AXES];
    for (axis_t a = 0; a < NUM_AXES; a++) {
        level_size_tiles[a] = level_size[a] * CHUNK_SIZE;
    }

    for (size_t z = 0; z < level_size_tiles[AXIS__Z]; z++) {
        for (size_t x = 0; x < level_size_tiles[AXIS__X]; x++) {
            size_t pos[NUM_AXES] = { x, level_size_tiles[AXIS__Y] - 1, z };

            tile_t tile;
            while ((tile = level_get_tile(level, pos)) == TILE__AIR) {
                pos[AXIS__Y]--;
            }

            bool sides_present[NUM_SIDES];
            look_up_sides(level, level_size_tiles, pos, sides_present);

            if (sides_present[SIDE__NORTH] + sides_present[SIDE__SOUTH] + sides_present[SIDE__WEST] + sides_present[SIDE__EAST] < 2) {
                level_set_tile(level, pos, TILE__AIR);
                pos[AXIS__Y]--;
                tile_t top_tile = TILE__GRASS;
                if (pos[AXIS__Y] <= 75) {
                    top_tile = TILE__SAND;
                }
                level_set_tile(level, pos, top_tile);
            }

            size_t const pos_below[NUM_AXES] = { pos[AXIS__X], pos[AXIS__Y] - 1, pos[AXIS__Z] };

            for (tile_shape_t i = 0; i < NUM_TILE_SHAPES; i++) {
                if (SHAPE_LOOKUP[i].defined) {
                    if (tile_matches_pattern(level, i, sides_present, pos)) {
                        level_set_tile_shape(level, pos, i);
                        if (i == TILE_SHAPE__CORNER_A_NORTH_WEST || i == TILE_SHAPE__CORNER_A_SOUTH_WEST || i == TILE_SHAPE__CORNER_A_NORTH_EAST || i == TILE_SHAPE__CORNER_A_SOUTH_EAST) {
                            tile_t top_tile = TILE__GRASS;
                            if (pos[AXIS__Y] - 1 <= 75) {
                                top_tile = TILE__SAND;
                            }
                            level_set_tile(level, pos_below, top_tile);

                            tile_shape_t below_shape_candidate = i + 4;
                            if (SHAPE_LOOKUP[below_shape_candidate].defined) {
                                bool below_sides_present[NUM_SIDES];
                                look_up_sides(level, level_size_tiles, pos_below, below_sides_present);
                                below_sides_present[SIDE__TOP] = false;
                                if (tile_matches_pattern(level, below_shape_candidate, below_sides_present, pos_below) ||
                                    (!below_sides_present[SIDE__NORTH] || !below_sides_present[SIDE__SOUTH] || !below_sides_present[SIDE__WEST] || !below_sides_present[SIDE__EAST])
                                ) {
                                    level_set_tile_shape(level, pos_below, below_shape_candidate);
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

static void look_up_sides(level_t const* const level, size_chunks_t const level_size_tiles[NUM_AXES], size_t const pos[NUM_AXES], bool sides[NUM_SIDES]) {
    assert(level != nullptr);

    for (side_t i = 0; i < NUM_SIDES; i++) {
        int offsets[NUM_AXES];
        side_get_offsets(i, offsets);
        int pos_t[NUM_AXES] = { pos[AXIS__X] + offsets[AXIS__X], pos[AXIS__Y] + offsets[AXIS__Y], pos[AXIS__Z] + offsets[AXIS__Z] };

        sides[i] = true;

        for (axis_t a = 0; a < NUM_AXES; a++) {
            if (pos_t[a] < 0 || pos_t[a] >= level_size_tiles[a]) {
                sides[i] = false;
                break;
            }
        }
        if (sides[i]) {
            size_t const pos_t_u[NUM_AXES] = { pos_t[AXIS__X], pos_t[AXIS__Y], pos_t[AXIS__Z] };
            tile_t const offset_tile = level_get_tile(level, pos_t_u);
            sides[i] = offset_tile != TILE__AIR;
        }
    }
}

static bool tile_matches_pattern(level_t const* const level, tile_shape_t const tile_shape, bool const sides[NUM_SIDES], size_t const pos[NUM_AXES]) {
    assert(level != nullptr);
    assert(tile_shape >= 0 && tile_shape < NUM_TILE_SHAPES);

    if (memcmp(sides, SHAPE_LOOKUP[tile_shape].sides_present, sizeof(bool) * NUM_SIDES) == 0) {
        if (SHAPE_LOOKUP[tile_shape].has_extra) {
            size_t const pos_north_west[NUM_AXES] = { pos[AXIS__X] - 1, pos[AXIS__Y], pos[AXIS__Z] - 1 };
            size_t const pos_south_west[NUM_AXES] = { pos[AXIS__X] + 1, pos[AXIS__Y], pos[AXIS__Z] - 1 };
            size_t const pos_north_east[NUM_AXES] = { pos[AXIS__X] - 1, pos[AXIS__Y], pos[AXIS__Z] + 1 };
            size_t const pos_south_east[NUM_AXES] = { pos[AXIS__X] + 1, pos[AXIS__Y], pos[AXIS__Z] + 1 };

            struct {
                bool north_west;
                bool south_west;
                bool north_east;
                bool south_east;
            } extra = {
                .north_west = level_get_tile(level, pos_north_west) != TILE__AIR,
                .south_west = level_get_tile(level, pos_south_west) != TILE__AIR,
                .north_east = level_get_tile(level, pos_north_east) != TILE__AIR,
                .south_east = level_get_tile(level, pos_south_east) != TILE__AIR
            };
            if (memcmp(&extra, &(SHAPE_LOOKUP[tile_shape].extra), sizeof(extra)) != 0) {
                return false;
            }
        }

        return true;
    }

    return false;
}
