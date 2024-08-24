#include "./aabb.h"

#include <assert.h>
#include <stdlib.h>

struct aabb {
    vec3 min;
    vec3 max;
};

aabb_t* const aabb_new(float const x_min, float const y_min, float const z_min, float const x_max, float const y_max, float const z_max) {
    aabb_t* self = malloc(sizeof(aabb_t));

    float _x_min = x_min;
    float _y_min = y_min;
    float _z_min = z_min;
    float _x_max = x_max;
    float _y_max = y_max;
    float _z_max = z_max;

    if (x_min > x_max) {
        _x_min = x_max;
        _x_max = x_min;
    }
    if (y_min > y_max) {
        _y_min = y_max;
        _y_max = y_min;
    }
    if (z_min > z_max) {
        _z_min = z_max;
        _z_max = z_min;
    }

    self->min[0] = _x_min;
    self->min[1] = _y_min;
    self->min[2] = _z_min;

    self->max[0] = _x_max;
    self->max[1] = _y_max;
    self->max[2] = _z_max;
    
    return self;
}

void aabb_delete(aabb_t* const self) {
    assert(self != nullptr);

    free(self);
}

void aabb_get_size(aabb_t const* const self, float size[3]) {
    assert(self != nullptr);
    assert(size != nullptr);

    size[0] = self->max[0] - self->min[0];
    size[1] = self->max[1] - self->min[1];
    size[2] = self->max[2] - self->min[2];
}

bool aabb_test_vec3_inside(aabb_t const* const self, vec3 const v) {
    assert(self != nullptr);

    return v[0] >= self->min[0] && v[0] <= self->max[0] &&
           v[1] >= self->min[1] && v[1] <= self->max[1] &&
           v[2] >= self->min[2] && v[2] <= self->max[2];
}

bool aabb_test_aabb_inside(aabb_t const* const self, aabb_t const* const other) {
    assert(self != nullptr);
    assert(other != nullptr);

    return self->min[0] <= other->min[0] && self->max[0] >= other->max[0] &&
           self->min[1] <= other->min[1] && self->max[1] >= other->max[1] &&
           self->min[2] <= other->min[2] && self->max[2] >= other->max[2];
}

bool aabb_test_aabb_overlap(aabb_t const* const self, aabb_t const* const other) {
    assert(self != nullptr);
    assert(other != nullptr);

    return self->min[0] <= other->max[0] && self->max[0] >= other->min[0] &&
           self->min[1] <= other->max[1] && self->max[1] >= other->min[1] &&
           self->min[2] <= other->max[2] && self->max[2] >= other->min[2];
}