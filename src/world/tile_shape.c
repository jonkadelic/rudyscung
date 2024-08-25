#include "./tile_shape.h"

#include <assert.h>

#include "side.h"

static bool const CAN_OCCLUDE[NUM_TILE_SHAPES][NUM_SIDES] = {
    [TILE_SHAPE__NO_RENDER] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = false,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__FLAT] = {
        [SIDE__NORTH] = true,
        [SIDE__SOUTH] = true,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = true,
        [SIDE__WEST] = true,
        [SIDE__EAST] = true
    },
    [TILE_SHAPE__RAMP_NORTH] = {
        [SIDE__NORTH] = true,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__RAMP_SOUTH] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = true,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__RAMP_WEST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = true,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__RAMP_EAST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = true
    },
    [TILE_SHAPE__CORNER_A_NORTH_WEST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = false,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__CORNER_A_SOUTH_WEST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = false,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__CORNER_A_NORTH_EAST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = false,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__CORNER_A_SOUTH_EAST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = false,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__CORNER_B_NORTH_WEST] = {
        [SIDE__NORTH] = true,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = true,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__CORNER_B_SOUTH_WEST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = true,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = true,
        [SIDE__EAST] = false
    },
    [TILE_SHAPE__CORNER_B_NORTH_EAST] = {
        [SIDE__NORTH] = true,
        [SIDE__SOUTH] = false,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = true
    },
    [TILE_SHAPE__CORNER_B_SOUTH_EAST] = {
        [SIDE__NORTH] = false,
        [SIDE__SOUTH] = true,
        [SIDE__BOTTOM] = true,
        [SIDE__TOP] = false,
        [SIDE__WEST] = false,
        [SIDE__EAST] = true
    },
};

bool const tile_shape_can_side_occlude(tile_shape_t const self, side_t const side) {
    assert(self >= 0 && self < NUM_TILE_SHAPES);
    assert(side >= 0 && side < NUM_SIDES);

    return CAN_OCCLUDE[self][side];
}