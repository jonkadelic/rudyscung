#pragma once

#include "./side.h"

typedef enum tile_shape {
    TILE_SHAPE__FLAT = 0,
    TILE_SHAPE__RAMP_NORTH = 1,
    TILE_SHAPE__RAMP_SOUTH = 2,
    TILE_SHAPE__RAMP_WEST = 3,
    TILE_SHAPE__RAMP_EAST = 4,
    // Inner corner - un-inverted triangle
    TILE_SHAPE__CORNER_A_NORTH_WEST = 5,
    TILE_SHAPE__CORNER_A_SOUTH_WEST = 6,
    TILE_SHAPE__CORNER_A_NORTH_EAST = 7,
    TILE_SHAPE__CORNER_A_SOUTH_EAST = 8,
    // Outer corner - inverted triangle
    TILE_SHAPE__CORNER_B_NORTH_WEST = 9,
    TILE_SHAPE__CORNER_B_SOUTH_WEST = 10,
    TILE_SHAPE__CORNER_B_NORTH_EAST = 11,
    TILE_SHAPE__CORNER_B_SOUTH_EAST = 12,
    NUM_TILE_SHAPES
} tile_shape_t;

bool const tile_shape_can_side_occlude(tile_shape_t const self, side_t const side);