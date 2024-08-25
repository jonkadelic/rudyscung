#include "./camera.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct camera {
    float pos[NUM_AXES];
    float rot[NUM_ROT_AXES];
};

camera_t* const camera_new(float const pos[NUM_AXES], float const rot[NUM_ROT_AXES]) {
    camera_t* const self = malloc(sizeof(camera_t));
    assert(self != nullptr);

    memcpy(self->pos, pos, sizeof(float) * NUM_AXES);
    memcpy(self->rot, rot, sizeof(float) * NUM_ROT_AXES);

    return self;
}

void camera_delete(camera_t* const self) {
    assert(self != nullptr);

    free(self);
}

void camera_get_pos(camera_t const* const self, float pos[NUM_AXES]) {
    assert(self != nullptr);

    memcpy(pos, self->pos, sizeof(float) * NUM_AXES);
}

void camera_set_pos(camera_t* const self, float const pos[NUM_AXES]) {
    assert(self != nullptr);

    memcpy(self->pos, pos, sizeof(float) * NUM_AXES);
}

void camera_get_rot(camera_t const* const self, float rot[NUM_ROT_AXES]) {
    assert(self != nullptr);

    memcpy(rot, self->rot, sizeof(float) * NUM_ROT_AXES);
}

void camera_set_rot(camera_t* const self, float const rot[NUM_ROT_AXES]) {
    assert(self != nullptr);

    memcpy(self->rot, rot, sizeof(float) * NUM_ROT_AXES);
}
