#include "./camera.h"

#include <assert.h>
#include <stdlib.h>

struct camera {
    float x;
    float y;
    float z;
    float y_rot;
    float x_rot;
};

camera_t* const camera_new(float const x, float const y, float const z) {
    camera_t* const self = malloc(sizeof(camera_t));
    assert(self != nullptr);

    self->x = x;
    self->y = y;
    self->z = z;
    self->y_rot = 0.0f;
    self->x_rot = 0.0f;

    return self;
}

void camera_delete(camera_t* const self) {
    assert(self != nullptr);

    free(self);
}

void camera_get_pos(camera_t const* const self, float pos[3]) {
    assert(self != nullptr);

    pos[0] = self->x;
    pos[1] = self->y;
    pos[2] = self->z;
}

void camera_set_pos(camera_t* const self, float const x, float const y, float const z) {
    assert(self != nullptr);

    self->x = x;
    self->y = y;
    self->z = z;
}

void camera_get_rot(camera_t const* const self, float rot[2]) {
    assert(self != nullptr);

    rot[0] = self->y_rot;
    rot[1] = self->x_rot;
}

void camera_set_rot(camera_t* const self, float const y_rot, float const x_rot) {
    assert(self != nullptr);

    self->y_rot = y_rot;
    self->x_rot = x_rot;
}
