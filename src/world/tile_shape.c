#include "./tile_shape.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>

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

typedef float const (*ramp_eq)(axis_t const axis, float const pos[NUM_AXES]);

static float const pos_ramp_north(axis_t const axis, float const pos[NUM_AXES]);
static float const pos_ramp_south(axis_t const axis, float const pos[NUM_AXES]);
static float const pos_ramp_west(axis_t const axis, float const pos[NUM_AXES]);
static float const pos_ramp_east(axis_t const axis, float const pos[NUM_AXES]);

static ramp_eq const RAMP_EQUATIONS[NUM_TILE_SHAPES] = {
    [TILE_SHAPE__RAMP_NORTH] = pos_ramp_north,
    [TILE_SHAPE__RAMP_SOUTH] = pos_ramp_south,
    [TILE_SHAPE__RAMP_WEST] = pos_ramp_west,
    [TILE_SHAPE__RAMP_EAST] = pos_ramp_east
};

bool const tile_shape_can_side_occlude(tile_shape_t const self, side_t const side) {
    assert(self >= 0 && self < NUM_TILE_SHAPES);
    assert(side >= 0 && side < NUM_SIDES);

    return CAN_OCCLUDE[self][side];
}

float const tile_shape_get_inner_distance(tile_shape_t const self, side_t const side, float const pos[2]) {
    assert(self >= 0 && self < NUM_TILE_SHAPES);
    assert(side >= 0 && side < NUM_SIDES);
    assert(pos[0] >= 0.0f && pos[0] < 1.0f);
    assert(pos[1] >= 0.0f && pos[1] < 1.0f);

    // Map [pos] into 3D space
    int o[NUM_AXES];
    side_get_offsets(side, o);
    float pos3[NUM_AXES];
    size_t i = 0;
    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (o[a] == -1) {
            pos3[a] = 1;
        } else if (o[a] == 1) {
            pos3[a] = 0;
        } else {
            pos3[a] = pos[i];
            i++;
        }
    }

    if (RAMP_EQUATIONS[self] != nullptr) {
        axis_t side_axis = side_get_axis(side);
        return RAMP_EQUATIONS[self](side_axis, pos3);
    }
    
    return 0.0f;
}

static float const pos_ramp_north(axis_t const axis, float const pos[NUM_AXES]) {
    assert(axis >= 0 && axis < NUM_AXES);

    // x + y - 1 = 0

    switch (axis) {
        case AXIS__X:
            // x = -(y - 1)
            return -(pos[AXIS__Y] - 1);
        case AXIS__Y:
            // y = -(x - 1)
            return -(pos[AXIS__X] - 1);
        case AXIS__Z:
            return INFINITY;
    }

    return 0.0f;
}

static float const pos_ramp_south(axis_t const axis, float const pos[NUM_AXES]) {
    assert(axis >= 0 && axis < NUM_AXES);

    // -x + y = 0

    switch (axis) {
        case AXIS__X:
            // x = y
            return pos[AXIS__Y];
        case AXIS__Y:
            // y = x
            return pos[AXIS__X];
        case AXIS__Z:
            return INFINITY;
    }

    return 0.0f;
}

static float const pos_ramp_west(axis_t const axis, float const pos[NUM_AXES]) {
    assert(axis >= 0 && axis < NUM_AXES);

    // y + z - 1 = 0

    switch (axis) {
        case AXIS__X:
            return INFINITY;
        case AXIS__Y:
            // y = -(z - 1)
            return -(pos[AXIS__Z] - 1);
        case AXIS__Z:
            // z = -(y - 1)
            return -(pos[AXIS__Y] - 1);
    }

    return 0.0f;
}

static float const pos_ramp_east(axis_t const axis, float const pos[NUM_AXES]) {
    assert(axis >= 0 && axis < NUM_AXES);

    // y - z = 0

    switch (axis) {
        case AXIS__X:
            return INFINITY;
        case AXIS__Y:
            // y = z
            return pos[AXIS__Z];
        case AXIS__Z:
            // z = y
            return pos[AXIS__Y];
    }

    return 0.0f;
}