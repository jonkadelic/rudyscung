#include "./tile.h"

#include <assert.h>
#include <stdlib.h>

#include "../util/logger.h"

static void tile_new(tile_t const self);

typedef struct tile_entry {
    uint8_t padding;
} tile_entry_t;

static tile_entry_t* TILES[NUM_TILES];

void tiles_init(void) {
    LOG_DEBUG("Initializing tiles...");
    
    for (size_t i = 0; i < NUM_TILES; i++) {
        TILES[i] = nullptr;
    }

    tile_new(TILE__AIR);
    tile_new(TILE__GRASS);
    tile_new(TILE__STONE);
    tile_new(TILE__SAND);
}

static void tile_new(tile_t const self) {
    assert(TILES[self] == nullptr);

    tile_entry_t* tile_entry = malloc(sizeof(tile_entry_t));
    assert(tile_entry != nullptr);

    TILES[self] = tile_entry;
}