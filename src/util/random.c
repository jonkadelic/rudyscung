#include "./random.h"

#include <assert.h>
#include <stdlib.h>

#include "src/util/object_counter.h"
#include "src/util/util.h"
#include "src/util/logger.h"

struct random {
    uint64_t seed;
};

static uint32_t const next(random_t* const self, size_t const bits);

random_t* const random_new(uint64_t const seed) {
    random_t* const self = malloc(sizeof(random_t));
    assert(self != nullptr);

    random_set_seed(self, seed);

    OBJ_CTR_INC(random_t);

    return self;
}

random_t* const random_new_from_time(void) {
    long seed = (long) get_time_ms();

    return random_new(seed);
}

void random_delete(random_t* const self) {
    assert(self != nullptr);

    free(self);

    OBJ_CTR_DEC(random_t);
}

void random_set_seed(random_t* const self, uint64_t const seed) {
    assert(self != nullptr);

    self->seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
}

void random_next_bytes(random_t* const self, size_t const num_bytes, uint8_t bytes[]) {
    assert(self != nullptr);
    assert(num_bytes > 0);

    for (size_t i = 0; i < num_bytes; i++) {
        for (int rnd = random_next_int(self), n = MIN(num_bytes, 4); n-- > 0; rnd >>= 8) {
            bytes[i++] = (uint8_t) rnd;
        }
    }
}

int const random_next_int(random_t* const self) {
    assert(self != nullptr);

    return next(self, 32);
}

uint32_t const random_next_int_bounded(random_t* const self, uint32_t const bound) {
    assert(self != nullptr);
    assert(bound > 0);

    if ((bound & -bound) == bound) {
        return (uint32_t)((bound * (uint64_t)next(self, 31)) >> 31);
    }

    uint32_t bits, val;
    do {
        bits = next(self, 31);
        val = bits % bound;
    } while (bits - val + (bound - 1) < 0);
    return val;
}

long const random_next_long(random_t* const self) {
    assert(self != nullptr);

    return ((long)next(self, 32) << 32) + next(self, 32);
}

bool const random_next_boolean(random_t* const self) {
    assert(self != nullptr);

    return next(self, 1) != 0;
}

float const random_next_float(random_t* const self) {
    assert(self != nullptr);

    return (next(self, 24) & 0x00FFFFFF) / ((float)(1 << 24));
}

double const random_next_double(random_t* const self) {
    assert(self != nullptr);

    return (((long)next(self, 26) << 27) + next(self, 27)) / (double) (1L << 53);
}

static uint32_t const next(random_t* const self, size_t const bits) {
    assert(self != nullptr);
    assert(bits <= 48);

    self->seed = (self->seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);

    return (uint32_t)(self->seed >> (48 - bits));
}