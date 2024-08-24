#include "./side.h"

#include <assert.h>

static int const OFFSETS[NUM_SIDES][3] = {
    [SIDE__NORTH] = { -1, 0, 0 },
    [SIDE__SOUTH] = { 1, 0, 0 },
    [SIDE__BOTTOM] = { 0, -1, 0 },
    [SIDE__TOP] = { 0, 1, 0 },
    [SIDE__WEST] = { 0, 0, -1 },
    [SIDE__EAST] = { 0, 0, 1 }
};

void side_get_offsets(side_t const self, int xyz[const 3]) {
    assert(self >= 0 && self < NUM_SIDES);
    assert(xyz != nullptr);

    xyz[0] = OFFSETS[self][0];
    xyz[1] = OFFSETS[self][1];
    xyz[2] = OFFSETS[self][2];
}

side_t side_get_opposite(side_t const self) {
    return self + (1 - ((self % 2) * 2));
}