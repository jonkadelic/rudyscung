#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum tile : uint8_t {
    TILE__AIR = 0,
    TILE__GRASS = 1,
    TILE__STONE = 2,
    TILE__SAND = 3,
    NUM_TILES
} tile_t;

void tiles_init(void);

void tiles_cleanup(void);
