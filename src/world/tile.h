#pragma once

#include <stddef.h>
#include <stdint.h>

#define NUM_TILES UINT8_MAX

typedef struct tile tile_t;

typedef enum tile_id : uint8_t {
    TILE_ID__AIR = 0,
    TILE_ID__GRASS = 1,
    TILE_ID__STONE = 2,
    TILE_ID__SAND = 3
} tile_id_t;

void tiles_init(void);

tile_t const* const tile_get(tile_id_t const id);

tile_id_t const tile_get_id(tile_t const* const self);
