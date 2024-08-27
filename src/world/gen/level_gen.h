#pragma once

#include "../chunk.h"
#include "../level.h"

typedef struct level_gen level_gen_t;

level_gen_t* const level_gen_new(uint64_t const seed);

void level_gen_delete(level_gen_t* const self);

void level_gen_generate(level_gen_t* const self, chunk_t* const chunk);

void level_gen_smooth(level_gen_t const* const self, level_t* const level);