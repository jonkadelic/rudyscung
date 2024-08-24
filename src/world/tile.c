#include "./tile.h"

#include <assert.h>
#include <stdlib.h>

static tile_t* tile_new(tile_id_t const id);

struct tile {
    tile_id_t id;
};

static tile_t const* TILES[NUM_TILES];

static tile_t const* tile_air;
static tile_t const* tile_grass;
static tile_t const* tile_stone;

void tiles_init(void) {
    for (size_t i = 0; i < NUM_TILES; i++) {
        TILES[i] = nullptr;
    }

    tile_air = tile_new(TILE_ID__AIR);
    tile_grass = tile_new(TILE_ID__GRASS);
    tile_stone = tile_new(TILE_ID__STONE);
}

tile_t const* const tile_get(tile_id_t const id) {
    assert(id >= 0 && id < NUM_TILES);
    assert(TILES[id] != nullptr);

    return TILES[id];
}

tile_id_t const tile_get_id(tile_t const* const self) {
    assert(self != nullptr);

    return self->id;
}

static tile_t* tile_new(tile_id_t const id) {
    assert(TILES[id] == nullptr);

    tile_t* tile = malloc(sizeof(tile_t));
    assert(tile != nullptr);

    tile->id = id;
    TILES[id] = tile;

    return tile;
}