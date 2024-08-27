#include "./camera.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../util.h"

typedef enum camera_type {
    CAMERA_TYPE__PERSPECTIVE,
    CAMERA_TYPE__ORTHO,
    NUM_CAMERA_TYPES
} camera_type_t;

typedef void (*set_matrices)(camera_t const* const self, size_t const window_size[2], shader_t* const shader);

struct camera {
    camera_type_t camera_type;
    float pos[NUM_AXES];
    float rot[NUM_ROT_AXES];
    set_matrices set_matrices;
};

static camera_t* const camera_new(camera_type_t const camera_type, set_matrices const set_matrices);

static void camera_perspective_set_matrices(camera_t const* const self, size_t const window_size[2], shader_t* const shader);
static void camera_ortho_set_matrices(camera_t const* const self, size_t const window_size[2], shader_t* const shader);

camera_perspective_t* const camera_perspective_new(void) {
    camera_t* self = camera_new(CAMERA_TYPE__PERSPECTIVE,  camera_perspective_set_matrices);

    return (camera_perspective_t*) self;
}

camera_ortho_t* const camera_ortho_new(void) {
    camera_t* self = camera_new(CAMERA_TYPE__ORTHO, camera_ortho_set_matrices);

    return (camera_ortho_t*) self;
}

void camera_delete(camera_t* const self) {
    assert(self != nullptr);

    free(self);
}

void camera_set_matrices(camera_t const* const self, size_t const window_size[2], shader_t* const shader) {
    assert(self != nullptr);
    assert(window_size[0] > 0);
    assert(window_size[1] > 0);
    assert(shader != nullptr);

    self->set_matrices(self, window_size, shader);
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

static camera_t* const camera_new(camera_type_t const camera_type, set_matrices const set_matrices) {
    assert(camera_type >= 0 && camera_type < NUM_CAMERA_TYPES);
    assert(set_matrices);

    camera_t* const self = malloc(sizeof(camera_t));
    assert(self != nullptr);

    self->camera_type = camera_type;
    self->pos[AXIS__X] = 0.0f;
    self->pos[AXIS__Y] = 0.0f;
    self->pos[AXIS__Z] = 0.0f;
    self->rot[ROT_AXIS__Y] = 0.0f;
    self->rot[ROT_AXIS__X] = 0.0f;
    self->set_matrices = set_matrices;

    return self;
}

static void camera_perspective_set_matrices(camera_t const* const self, size_t const window_size[2], shader_t* const shader) {
    assert(self != nullptr);
    assert(window_size[0] > 0);
    assert(window_size[1] > 0);
    assert(shader != nullptr);

    mat4x4 mat_view;
    mat4x4_identity(mat_view);
    mat4x4_rotate(mat_view, mat_view, 1.0f, 0.0f, 0.0f, self->rot[ROT_AXIS__X]);
    mat4x4_rotate(mat_view, mat_view, 0.0f, 1.0f, 0.0f, self->rot[ROT_AXIS__Y]);
    mat4x4_translate_in_place(mat_view, -self->pos[AXIS__X], -self->pos[AXIS__Y], -self->pos[AXIS__Z]);
    shader_put_uniform_mat4x4(shader, "view", mat_view);

    mat4x4 mat_proj;
    mat4x4_identity(mat_proj);
    mat4x4_perspective(mat_proj, TO_RADIANS(65.0f), (float) window_size[0] / window_size[1], 0.1f, 1000.0f);
    shader_put_uniform_mat4x4(shader, "projection", mat_proj);
}

static void camera_ortho_set_matrices(camera_t const* const self, size_t const window_size[2], shader_t* const shader) {
    assert(self != nullptr);
    assert(window_size[0] > 0);
    assert(window_size[1] > 0);
    assert(shader != nullptr);

    float scale = pow(2, 128.0f - self->pos[AXIS__Y]);
    mat4x4 mat_view;
    mat4x4_identity(mat_view);
    mat4x4_translate_in_place(mat_view, window_size[0] / scale / 2.0f, window_size[1] / scale / 2.0f, -2000.0f);
    mat4x4_scale_aniso(mat_view, mat_view, 1, -1, 1);
    mat4x4_rotate(mat_view, mat_view, 1.0f, 0.0f, 0.0f, 1 * (M_PI / 4));
    mat4x4_rotate(mat_view, mat_view, 0.0f, 1.0f, 0.0f, self->rot[ROT_AXIS__Y]);
    mat4x4_translate_in_place(mat_view, -self->pos[AXIS__X], -64.0f, -self->pos[AXIS__Z]);
    shader_put_uniform_mat4x4(shader, "view", mat_view);

    mat4x4 mat_proj;
    mat4x4_identity(mat_proj);
    mat4x4_ortho(mat_proj, 0, window_size[0] / scale, window_size[1] / scale, 0, 0.0f, 3000.0f);
    shader_put_uniform_mat4x4(shader, "projection", mat_proj);
}