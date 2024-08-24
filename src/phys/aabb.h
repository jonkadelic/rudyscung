#pragma once

#include "../linmath.h"

typedef struct aabb aabb_t;

aabb_t* const aabb_new(float const x_min, float const y_min, float const z_min, float const x_max, float const y_max, float const z_max);

void aabb_delete(aabb_t* const self);

void aabb_get_size(aabb_t const* const self, float size[3]);

bool aabb_test_vec3_inside(aabb_t const* const self, vec3 const v);

bool aabb_test_aabb_inside(aabb_t const* const self, aabb_t const* const other);

bool aabb_test_aabb_overlap(aabb_t const* const self, aabb_t const* const other);
