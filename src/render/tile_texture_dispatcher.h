#pragma once

#include "../world/tile.h"

typedef size_t tile_texture_coord_t;

typedef struct tile_texture_coords {
    union {
        struct {
            tile_texture_coord_t x;
            tile_texture_coord_t y;
        };
        tile_texture_coord_t values[2];
    };
} tile_texture_coords_t;

void tile_texture_dispatcher_get_tile_texture_coords(tile_t const* const tile, tile_texture_coords_t* const texture_coords);