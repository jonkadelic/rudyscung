#pragma once

#include "src/world/side.h"

typedef struct aabb aabb_t;

aabb_t* const aabb_new(float const min[NUM_AXES], float const max[NUM_AXES]);

aabb_t* const aabb_new_default(void);

void aabb_delete(aabb_t* const self);

void aabb_set_bounds(aabb_t* const self, float const min[NUM_AXES], float const max[NUM_AXES]);

void aabb_get_size(aabb_t const* const self, float size[NUM_AXES]);

void aabb_translate(aabb_t const* const self, float const offset[NUM_AXES], aabb_t* const out);

bool aabb_test_pos_inside(aabb_t const* const self, float const v[NUM_AXES]);

bool aabb_test_aabb_inside(aabb_t const* const self, aabb_t const* const other);

bool aabb_test_aabb_overlap(aabb_t const* const self, aabb_t const* const other);

void aabb_get_point(aabb_t const* const self, side_t const sides[NUM_AXES], float pos[NUM_AXES]);