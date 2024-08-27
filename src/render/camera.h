#pragma once

#include "../world/side.h"
#include "./shader.h"

typedef struct camera camera_t;

typedef struct camera camera_perspective_t;
typedef struct camera camera_ortho_t;

camera_perspective_t* const camera_perspective_new(void);

camera_ortho_t* const camera_ortho_new(void);

void camera_delete(camera_t* const self);

void camera_set_matrices(camera_t const* const self, size_t const window_size[2], shader_t* const shader);

void camera_get_pos(camera_t const* const self, float pos[NUM_AXES]);

void camera_set_pos(camera_t* const self, float const pos[NUM_AXES]);

void camera_get_rot(camera_t const* const self, float rot[NUM_ROT_AXES]);

void camera_set_rot(camera_t* const self, float const rot[NUM_ROT_AXES]);
