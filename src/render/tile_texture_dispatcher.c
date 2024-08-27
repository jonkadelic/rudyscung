#include "./tile_texture_dispatcher.h"

#include <assert.h>

typedef struct entry {
    bool exists;
    bool is_callable;
    union {
        tile_texture_coords_t texture_coords[NUM_SIDES];
        void (*callable)(tile_texture_coords_t* const texture_coords, side_t const side);
    };
} entry_t;

static entry_t const ENTRIES[NUM_TILES] = {
    [TILE__GRASS] = {
        .exists = true,
        .is_callable = false,
        .texture_coords = {
            [SIDE__NORTH] = { .x = 1, .y = 0 },
            [SIDE__SOUTH] = { .x = 1, .y = 0 },
            [SIDE__BOTTOM] = { .x = 1, .y = 0 },
            [SIDE__TOP] = { .x = 0, .y = 0 },
            [SIDE__WEST] = { .x = 1, .y = 0 },
            [SIDE__EAST] = { .x = 1, .y = 0 },
        }
    },
    [TILE__STONE] = {
        .exists = true,
        .is_callable = false,
        .texture_coords = {
            [SIDE__NORTH] = { .x = 1, .y = 0 },
            [SIDE__SOUTH] = { .x = 1, .y = 0 },
            [SIDE__BOTTOM] = { .x = 1, .y = 0 },
            [SIDE__TOP] = { .x = 1, .y = 0 },
            [SIDE__WEST] = { .x = 1, .y = 0 },
            [SIDE__EAST] = { .x = 1, .y = 0 },
        }
    },
    [TILE__SAND] = {
        .exists = true,
        .is_callable = false,
        .texture_coords = {
            [SIDE__NORTH] = { .x = 1, .y = 0 },
            [SIDE__SOUTH] = { .x = 1, .y = 0 },
            [SIDE__BOTTOM] = { .x = 1, .y = 0 },
            [SIDE__TOP] = { .x = 2, .y = 1 },
            [SIDE__WEST] = { .x = 1, .y = 0 },
            [SIDE__EAST] = { .x = 1, .y = 0 },
        }
    }
};

void tile_texture_dispatcher_get_tile_texture_coords(tile_t const tile, side_t const side, tile_texture_coords_t* const texture_coords) {
    assert(tile >= 0 && tile < NUM_TILES);
    assert(side >= 0 && side < NUM_SIDES);
    assert(texture_coords != nullptr);

    entry_t const* const entry = &(ENTRIES[tile]);

    if (entry->exists) {
        if (entry->is_callable) {
            entry->callable(texture_coords, side);
        } else {
            texture_coords->x = entry->texture_coords[side].x;
            texture_coords->y = entry->texture_coords[side].y;
        }
    } else {
        texture_coords->x = 0;
        texture_coords->y = 0;
    }
}