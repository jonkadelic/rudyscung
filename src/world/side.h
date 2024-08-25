#pragma once

typedef enum side {
    SIDE__NORTH = 0,  // -X
    SIDE__SOUTH = 1,  // +X
    SIDE__BOTTOM = 2, // -Y 
    SIDE__TOP = 3,    // +Y
    SIDE__WEST = 4,   // -Z
    SIDE__EAST = 5,   // +Z
    NUM_SIDES
} side_t;

typedef enum axis {
    AXIS__X = 0,
    AXIS__Y = 1,
    AXIS__Z = 2,
    NUM_AXES
} axis_t;

typedef enum rot_axis {
    ROT_AXIS__Y = 0,
    ROT_AXIS__X = 1,
    NUM_ROT_AXES
} rot_axis_t;

void side_get_offsets(side_t const self, int xyz[NUM_AXES]);

side_t const side_get_opposite(side_t const self);

axis_t const side_get_axis(side_t const self);