#include "./perlin.h"

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "../../rand.h"

struct perlin {
    int p[512];
};

static double fade(double t);

static double lerp(double t, double a, double b);

static double grad(int hash, double x, double y, double z);

perlin_t* const perlin_new(uint64_t const seed) {
    perlin_t* const self = malloc(sizeof(perlin_t));
    assert(self != nullptr);

    // Set up random
    random_t* rand = random_new(seed);

    // Assign initial values for each permutation
    for (size_t i = 0; i < 256; i++) {
        self->p[i] = i;
    }

    // Shuffle values
    for (size_t i = 0; i < 256; i++) {
        size_t new_i = random_next_int_bounded(rand, 256 - i) + i;
        int temp = self->p[i];
        self->p[i] = self->p[new_i];
        self->p[new_i] = temp;
        self->p[i + 256] = self->p[i];
    }

    random_delete(rand);

    return self;
}

void perlin_delete(perlin_t* const self) {
    assert(self != nullptr);

    free(self);
}

double perlin_get_1d(perlin_t const* const self, double x) {
    return perlin_get_2d(self, x, 0);
}

double perlin_get_2d(perlin_t const* const self, double x, double y) {
    return perlin_get_3d(self, x, y, 0);
}

double perlin_get_3d(perlin_t const* const self, double x, double y, double z) {
    assert(self != nullptr);

    int X = (int)floor(x) & 255,                  // FIND UNIT CUBE THAT
        Y = (int)floor(y) & 255,                  // CONTAINS POINT.
        Z = (int)floor(z) & 255;
    x -= floor(x);                                // FIND RELATIVE X,Y,Z
    y -= floor(y);                                // OF POINT IN CUBE.
    z -= floor(z);
    double u = fade(x),                                // COMPUTE FADE CURVES
           v = fade(y),                                // FOR EACH OF X,Y,Z.
           w = fade(z);

    int A = self->p[X  ]+Y, AA = self->p[A]+Z, AB = self->p[A+1]+Z,      // HASH COORDINATES OF
        B = self->p[X+1]+Y, BA = self->p[B]+Z, BB = self->p[B+1]+Z;      // THE 8 CUBE CORNERS,

    return lerp(w, lerp(v, lerp(u, grad(self->p[AA  ], x  , y  , z   ),  // AND ADD
                                   grad(self->p[BA  ], x-1, y  , z   )), // BLENDED
                           lerp(u, grad(self->p[AB  ], x  , y-1, z   ),  // RESULTS
                                   grad(self->p[BB  ], x-1, y-1, z   ))),// FROM  8
                   lerp(v, lerp(u, grad(self->p[AA+1], x  , y  , z-1 ),  // CORNERS
                                   grad(self->p[BA+1], x-1, y  , z-1 )), // OF CUBE
                           lerp(u, grad(self->p[AB+1], x  , y-1, z-1 ),
                                   grad(self->p[BB+1], x-1, y-1, z-1 ))));
}

static double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static double lerp(double t, double a, double b) {
    return a + t * (b - a);
}

static double grad(int hash, double x, double y, double z) {
        int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
        double u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
               v = h<4 ? y : h==12||h==14 ? x : z;
        return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}
