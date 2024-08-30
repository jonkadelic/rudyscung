#include "./server.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "src/util/logger.h"
#include "src/util/util.h"
#include "src/world/chunk.h"
#include "src/world/level.h"
#include "src/world/side.h"

#define TICKS_PER_SECOND 20
#define MS_PER_TICK (1000 / (TICKS_PER_SECOND))

struct server {

};

static void tick(server_t* const self, level_t* const level);

server_t* const server_new() {
    server_t* self = malloc(sizeof(server_t));
    assert(self != nullptr);

    LOG_DEBUG("server_t: initialized.");

    return self;
}

void server_delete(server_t* const self) {
    assert(self != nullptr);

    free(self);

    LOG_DEBUG("server_t: deleted.");
}

void server_run(server_t* const self) {
    assert(self != nullptr);

#define LEVEL_SIZE 16
#define LEVEL_HEIGHT 8
    level_t* const level = level_new((size_chunks_t[NUM_AXES]) { LEVEL_SIZE, LEVEL_SIZE, LEVEL_HEIGHT });

    bool running = true;
    uint64_t last_game_tick = get_time_ms();
    while (running) {
        uint64_t const current_tick = get_time_ms();
        uint64_t const delta_tick = current_tick - last_game_tick;

        // Game tick
        size_t ticks = (size_t) (delta_tick / MS_PER_TICK);
        for (size_t t = 0; t < ticks; t++) {
            tick(self, level);
        }
        last_game_tick = current_tick - (delta_tick % MS_PER_TICK);
    }

    level_delete(level);
}

static void tick(server_t* const self, level_t* const level) {
    assert(self != nullptr);
    assert(level != nullptr);

    level_tick(level);
}
