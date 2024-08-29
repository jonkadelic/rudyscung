#include "./aabb.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "src/world/side.h"

struct aabb {
    float min[NUM_AXES];
    float max[NUM_AXES];
};

aabb_t* const aabb_new(float const min[NUM_AXES], float const max[NUM_AXES]) {
    aabb_t* self = malloc(sizeof(aabb_t));

    aabb_set_bounds(self, min, max);
    
    return self;
}

aabb_t* const aabb_new_default(void) {
    return aabb_new((float[NUM_AXES]) { 0.0f, 0.0f, 0.0f }, (float[NUM_AXES]) { 1.0f, 1.0f, 1.0f });
}

void aabb_delete(aabb_t* const self) {
    assert(self != nullptr);

    free(self);
}

void aabb_set_bounds(aabb_t* const self, float const min[NUM_AXES], float const max[NUM_AXES]) {
    assert(self != nullptr);

    float _min[NUM_AXES];
    memcpy(_min, min, sizeof(float) * NUM_AXES);
    float _max[NUM_AXES];
    memcpy(_max, max, sizeof(float) * NUM_AXES);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (_max[a] < _min[a]) {
            float temp = _min[a];
            _min[a] = _max[a];
            _max[a] = temp;

        }
    }

    memcpy(self->min, _min, sizeof(float) * NUM_AXES);
    memcpy(self->max, _max, sizeof(float) * NUM_AXES);
}

void aabb_get_size(aabb_t const* const self, float size[NUM_AXES]) {
    assert(self != nullptr);
    assert(size != nullptr);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        size[a] = self->max[a] - self->min[a];
    }
}

void aabb_translate(aabb_t const* const self, float const offset[NUM_AXES], aabb_t* const out) {
    assert(self != nullptr);
    assert(out != nullptr);

    aabb_set_bounds(out,
        (float[NUM_AXES]) {
            self->min[AXIS__X] + offset[AXIS__X],
            self->min[AXIS__Y] + offset[AXIS__Y],
            self->min[AXIS__Z] + offset[AXIS__Z]
        },
        (float[NUM_AXES]) {
            self->max[AXIS__X] + offset[AXIS__X],
            self->max[AXIS__Y] + offset[AXIS__Y],
            self->max[AXIS__Z] + offset[AXIS__Z]
        }
    );
}

bool aabb_test_pos_inside(aabb_t const* const self, float const v[NUM_AXES]) {
    assert(self != nullptr);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (!(v[a] >= self->min[a] && v[a] <= self->max[a])) {
            return false;
        }
    }

    return true;
}

bool aabb_test_aabb_inside(aabb_t const* const self, aabb_t const* const other) {
    assert(self != nullptr);
    assert(other != nullptr);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (!(self->min[a] <= other->min[a] && self->max[a] >= other->max[a])) {
            return false;
        }
    }

    return true;
}

bool aabb_test_aabb_overlap(aabb_t const* const self, aabb_t const* const other) {
    assert(self != nullptr);
    assert(other != nullptr);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (!(self->min[a] <= other->max[a] && self->max[a] >= other->min[a])) {
            return false;
        }
    }

    return true;
}

void aabb_get_point(aabb_t const* const self, side_t const sides[NUM_AXES], float pos[NUM_AXES]) {
    assert(self != nullptr);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        int o[NUM_AXES];
        side_get_offsets(sides[a], o);
        if (o[a] < 0) {
            pos[a] = self->min[a];
        } else {
            pos[a] = self->max[a];
        }
    }
}