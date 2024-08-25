#pragma once

#include "../world/side.h"

typedef struct camera camera_t;

camera_t* const camera_new(float const pos[NUM_AXES], float const rot[NUM_ROT_AXES]);

void camera_delete(camera_t* const self);

void camera_get_pos(camera_t const* const self, float pos[NUM_AXES]);

void camera_set_pos(camera_t* const self, float const pos[NUM_AXES]);

void camera_get_rot(camera_t const* const self, float rot[NUM_ROT_AXES]);

void camera_set_rot(camera_t* const self, float const rot[NUM_ROT_AXES]);
