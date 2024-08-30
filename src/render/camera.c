#include "./camera.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/cglm.h>

#include "src/render/gl.h"
#include "src/util/object_counter.h"
#include "src/util/util.h"
#include "src/util/logger.h"
#include "src/world/side.h"

typedef enum camera_type {
    CAMERA_TYPE__PERSPECTIVE,
    CAMERA_TYPE__ORTHO,
    NUM_CAMERA_TYPES
} camera_type_t;

typedef void (*set_matrices)(camera_t* const self, size_t const window_size[2], shader_t* const shader);

struct camera {
    camera_type_t camera_type;
    float pos[NUM_AXES];
    float rot[NUM_ROT_AXES];
    mat4 mat_proj;
    mat4 mat_view;
    set_matrices set_matrices;
};

static camera_t* const camera_new(camera_type_t const camera_type, set_matrices const set_matrices);

static void camera_perspective_set_matrices(camera_t* const self, size_t const window_size[2], shader_t* const shader);
static void camera_ortho_set_matrices(camera_t* const self, size_t const window_size[2], shader_t* const shader);

camera_perspective_t* const camera_perspective_new(void) {
    camera_t* self = camera_new(CAMERA_TYPE__PERSPECTIVE,  camera_perspective_set_matrices);

    OBJ_CTR_INC(camera_t);

    return (camera_perspective_t*) self;
}

camera_ortho_t* const camera_ortho_new(void) {
    camera_t* self = camera_new(CAMERA_TYPE__ORTHO, camera_ortho_set_matrices);

    OBJ_CTR_INC(camera_t);

    return (camera_ortho_t*) self;
}

void camera_delete(camera_t* const self) {
    assert(self != nullptr);

    free(self);

    OBJ_CTR_DEC(camera_t);
}

void camera_set_matrices(camera_t* const self, size_t const window_size[2], shader_t* const shader) {
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

bool const camera_pick(camera_t* const self, size_t const window_size[2], size_t const screen_pos[2], float world_pos[NUM_AXES]) {
    assert(self != nullptr);
    assert(window_size[0] > 0);
    assert(window_size[1] > 0);
    assert(screen_pos[0] >= 0 && screen_pos[0] < window_size[0]);
    assert(screen_pos[1] >= 0 && screen_pos[1] < window_size[1]);

    float depth = 0.0f;
    glReadPixels(screen_pos[0], screen_pos[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    if (depth >= 1.0f) {
        return false;
    }

    float screen_vec[3] = {
        screen_pos[0],
        screen_pos[1],
        depth
    };

    mat4 mat_view_proj;
    glm_mat4_mul(self->mat_proj, self->mat_view, mat_view_proj);

    vec4 viewport = { 0.0f, 0.0f, window_size[0], window_size[1] };
    vec3 pos;
    glm_unproject(screen_vec, mat_view_proj, viewport, pos);

    vec4 pos4 = { 0.0f, 0.0f, 0.1f, 1.0f };
    mat4 slightly_further;
    glm_mat4_identity(slightly_further);
    glm_rotate(slightly_further, 1 * (M_PI / 4), (vec3) { 1.0f, 0.0f, 0.0f });
    glm_rotate(slightly_further, self->rot[ROT_AXIS__Y], (vec3) { 0.0f, 1.0f, 0.0f });
    glm_mat4_mulv(slightly_further, pos4, pos4);

    world_pos[0] = pos[0] + pos4[0];
    world_pos[1] = pos[1] - pos4[1];
    world_pos[2] = pos[2] - pos4[2];

    return true;
}

void camera_get_pos_for_sprites(camera_t const* const self, float pos[NUM_AXES]) {
    assert(self != nullptr);

    switch (self->camera_type) {
        case CAMERA_TYPE__PERSPECTIVE: {
            camera_get_pos(self, pos);
            break;
        }
        case CAMERA_TYPE__ORTHO: {
            vec4 pos4 = { 0.0f, 0.0f, -1000.0f, 1.0f };
            mat4 slightly_further;
            glm_mat4_identity(slightly_further);
            glm_rotate(slightly_further, -self->rot[ROT_AXIS__Y], (vec3) { 0.0f, 1.0f, 0.0f });
            glm_mat4_mulv(slightly_further, pos4, pos4);

            pos[AXIS__X] = self->pos[AXIS__X] - pos4[0];
            pos[AXIS__Y] = self->pos[AXIS__Y] - pos4[1];
            pos[AXIS__Z] = self->pos[AXIS__Z] - pos4[2];

            break;
        }
        default:
            assert(false);
    }
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

    glm_mat4_identity(self->mat_proj);
    glm_mat4_identity(self->mat_view);

    return self;
}

static void camera_perspective_set_matrices(camera_t* const self, size_t const window_size[2], shader_t* const shader) {
    assert(self != nullptr);
    assert(window_size[0] > 0);
    assert(window_size[1] > 0);
    assert(shader != nullptr);

    glm_mat4_identity(self->mat_view);
    glm_rotate(self->mat_view, self->rot[ROT_AXIS__X], (vec3) { 1.0f, 0.0f, 0.0f });
    glm_rotate(self->mat_view, self->rot[ROT_AXIS__Y], (vec3) { 0.0f, 1.0f, 0.0f });
    glm_translate(self->mat_view, (vec3) { -self->pos[AXIS__X], -self->pos[AXIS__Y], -self->pos[AXIS__Z] });
    shader_put_uniform_mat4(shader, "view", self->mat_view);

    glm_mat4_identity(self->mat_proj);
    glm_perspective(TO_RADIANS(65.0f), (float) window_size[0] / window_size[1], 0.1f, 1000.0f, self->mat_proj);
    shader_put_uniform_mat4(shader, "projection", self->mat_proj);
}

static void camera_ortho_set_matrices(camera_t* const self, size_t const window_size[2], shader_t* const shader) {
    assert(self != nullptr);
    assert(window_size[0] > 0);
    assert(window_size[1] > 0);
    assert(shader != nullptr);

    float scale = pow(2, 128.0f - self->pos[AXIS__Y]);
    glm_mat4_identity(self->mat_view);
    glm_translate(self->mat_view, (vec3) { window_size[0] / scale / 2.0f, window_size[1] / scale / 2.0f, -2000.0f });
    glm_scale(self->mat_view, (vec3) { 1, -1, 1 });
    glm_rotate(self->mat_view, 1 * (M_PI / 4), (vec3) { 1.0f, 0.0f, 0.0f });
    glm_rotate(self->mat_view, self->rot[ROT_AXIS__Y], (vec3) { 0.0f, 1.0f, 0.0f });
    glm_translate(self->mat_view, (vec3) { -self->pos[AXIS__X], -64.0f, -self->pos[AXIS__Z] });
    shader_put_uniform_mat4(shader, "view", self->mat_view);

    glm_mat4_identity(self->mat_proj);
    glm_ortho(0, window_size[0] / scale, window_size[1] / scale, 0, 0.0f, 3000.0f, self->mat_proj);
    shader_put_uniform_mat4(shader, "projection", self->mat_proj);
}