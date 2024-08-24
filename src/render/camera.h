#pragma once

typedef struct camera camera_t;

camera_t* const camera_new(float const x, float const y, float const z);

void camera_delete(camera_t* const self);

void camera_get_pos(camera_t const* const self, float pos[3]);

void camera_set_pos(camera_t* const self, float const x, float const y, float const z);

void camera_get_rot(camera_t const* const self, float rot[2]);

void camera_set_rot(camera_t* const self, float const y_rot, float const x_rot);
