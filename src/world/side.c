#include "./side.h"

#include <assert.h>
#include <stddef.h>

static int const OFFSETS[NUM_SIDES][NUM_AXES] = {
    [SIDE__NORTH] = { -1, 0, 0 },
    [SIDE__SOUTH] = { 1, 0, 0 },
    [SIDE__BOTTOM] = { 0, -1, 0 },
    [SIDE__TOP] = { 0, 1, 0 },
    [SIDE__WEST] = { 0, 0, -1 },
    [SIDE__EAST] = { 0, 0, 1 }
};

static axis_t const AXES[NUM_SIDES] = {
    [SIDE__NORTH] = AXIS__X,
    [SIDE__SOUTH] = AXIS__X,
    [SIDE__BOTTOM] = AXIS__Y,
    [SIDE__TOP] = AXIS__Y,
    [SIDE__WEST] = AXIS__Z,
    [SIDE__EAST] = AXIS__Z
};

void side_get_offsets(side_t const self, int xyz[const NUM_AXES]) {
    assert(self >= 0 && self < NUM_SIDES);
    assert(xyz != nullptr);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        xyz[a] = OFFSETS[self][a];
    }
}

side_t const side_get_opposite(side_t const self) {
    assert(self >= 0 && self < NUM_SIDES);

    return self + (1 - ((self % 2) * 2));
}

axis_t const side_get_axis(side_t const self) {
    assert(self >= 0 && self < NUM_SIDES);

    return AXES[self];
}

void side_map_point(side_t const self, float const pos3[NUM_AXES], float pos2[2]) {
    assert(self >= 0 && self < NUM_SIDES);

    axis_t axis = side_get_axis(self);

    size_t i = 0;
    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (a != axis) {
            pos2[i] = pos3[a];
            i++;
        }
    }
}