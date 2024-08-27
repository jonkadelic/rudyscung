#pragma once

#include <stdint.h>

typedef struct perlin perlin_t;

perlin_t* const perlin_new(uint64_t const seed);

void perlin_delete(perlin_t* const self);

double perlin_get_1d(perlin_t const* const self, double const x);

double perlin_get_2d(perlin_t const* const self, double const x, double const y);

double perlin_get_3d(perlin_t const* const self, double const x, double const y, double const z);
