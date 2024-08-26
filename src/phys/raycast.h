#pragma once

#include <stddef.h>

#include "../world/side.h"
#include "../world/tile.h"
#include "../world/level.h"

typedef struct raycast {
    bool hit;
    float pos[NUM_AXES];
    size_t tile_pos[NUM_AXES];
    tile_t tile;
    side_t side;
} raycast_t;

void raycast_cast_in_level(raycast_t* const self, level_t const* const level, float const pos[NUM_ROT_AXES], float const rot[NUM_ROT_AXES]);