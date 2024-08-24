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

void side_get_offsets(side_t const self, int xyz[const 3]);

side_t side_get_opposite(side_t const self);