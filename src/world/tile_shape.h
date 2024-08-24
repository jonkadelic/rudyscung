#pragma once

#include "./side.h"

typedef enum tile_shape {
    TILE_SHAPE__NO_RENDER,
    TILE_SHAPE__FLAT,
    TILE_SHAPE__RAMP_NORTH,
    TILE_SHAPE__RAMP_SOUTH,
    TILE_SHAPE__RAMP_WEST,
    TILE_SHAPE__RAMP_EAST,
    // Inner corner - un-inverted triangle
    TILE_SHAPE__CORNER_A_NORTH_WEST,
    TILE_SHAPE__CORNER_A_SOUTH_WEST,
    TILE_SHAPE__CORNER_A_NORTH_EAST,
    TILE_SHAPE__CORNER_A_SOUTH_EAST,
    // Outer corner - inverted triangle
    TILE_SHAPE__CORNER_B_NORTH_WEST,
    TILE_SHAPE__CORNER_B_SOUTH_WEST,
    TILE_SHAPE__CORNER_B_NORTH_EAST,
    TILE_SHAPE__CORNER_B_SOUTH_EAST,
    NUM_TILE_SHAPES
} tile_shape_t;

bool const tile_shape_can_side_occlude(tile_shape_t const self, side_t const side);