#include "./sprites.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <cglm/cglm.h>

#include "src/world/side.h"
#include "tessellator.h"
#include "textures.h"
#include "../rudyscung.h"
#include "./camera.h"
#include "../util.h"

typedef enum sprite_angle {
    SPRITE_ANGLE__FRONT,
    SPRITE_ANGLE__BACK,
    SPRITE_ANGLE__LEFT,
    SPRITE_ANGLE__RIGHT,
    NUM_SPRITE_ANGLES
} sprite_angle_t;

typedef struct sprite_entry {
    GLuint tex[NUM_SPRITE_ANGLES];
    size_t size[2];
    float origin[2];
    GLuint vao;
    GLuint vbo;
    size_t num_elements;
} sprite_entry_t;

struct sprites {
    rudyscung_t* rudyscung;
    sprite_entry_t sprites[NUM_SPRITES];
};

static void sprite_new(sprites_t* const self, sprite_t const sprite, textures_t* const textures, tessellator_t* tessellator, texture_name_t const texture_names[NUM_SPRITE_ANGLES], size_t size[2], float origin[2]);

sprites_t* const sprites_new(rudyscung_t* const rudyscung) {
    assert(rudyscung != nullptr);

    sprites_t* self = malloc(sizeof(sprites_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;

    textures_t* const textures = rudyscung_get_textures(rudyscung);
    tessellator_t* const tessellator = tessellator_new();

    sprite_new(self, 
        SPRITE__TREE,
        textures,
        tessellator,
        (texture_name_t[NUM_SPRITE_ANGLES]) {
            [SPRITE_ANGLE__FRONT] = TEXTURE_NAME__SPRITE_TREE,
            [SPRITE_ANGLE__BACK] = TEXTURE_NAME__SPRITE_TREE,
            [SPRITE_ANGLE__LEFT] = TEXTURE_NAME__SPRITE_TREE,
            [SPRITE_ANGLE__RIGHT] = TEXTURE_NAME__SPRITE_TREE
        }, 
        (size_t[2]) { 256, 256 }, 
        (float[2]) { 0.5f, 1.0f }
    );
    sprite_new(self,
        SPRITE__MOB,
        textures,
        tessellator,
        (texture_name_t[NUM_SPRITE_ANGLES]) {
            [SPRITE_ANGLE__FRONT] = TEXTURE_NAME__SPRITE_MOB_FRONT,
            [SPRITE_ANGLE__BACK] = TEXTURE_NAME__SPRITE_MOB_BACK,
            [SPRITE_ANGLE__LEFT] = TEXTURE_NAME__SPRITE_MOB_LEFT,
            [SPRITE_ANGLE__RIGHT] = TEXTURE_NAME__SPRITE_MOB_RIGHT
        },
        (size_t[2]) { 16, 32 },
        (float[2]) { 0.5f, 1.0f }
    );

    tessellator_delete(tessellator);

    return self;
}

void sprites_delete(sprites_t* const self) {
    assert(self != nullptr);

    for (sprite_t sprite = 0; sprite < NUM_SPRITES; sprite++) {
        glDeleteBuffers(1, &(self->sprites[sprite].vbo));
        glDeleteVertexArrays(1, &(self->sprites[sprite].vao));
    }

    free(self);
}

void sprites_get_size(sprites_t const* const self, sprite_t const sprite, size_t size[2]) {
    assert(self != nullptr);
    assert(sprite >= 0 && sprite < NUM_SPRITES);

    memcpy(size, self->sprites[sprite].size, sizeof(size_t) * 2);
}

void sprites_get_origin(sprites_t const* const self, sprite_t const sprite, float origin[2]) {
    assert(self != nullptr);
    assert(sprite >= 0 && sprite < NUM_SPRITES);

    memcpy(origin, self->sprites[sprite].origin, sizeof(float) * 2);
}

void sprites_render(sprites_t const* const self, sprite_t const sprite, camera_t* const camera, float const scale, float const pos[NUM_AXES], float const rotation_offset, bool const rotate[NUM_ROT_AXES]) {
    assert(self != nullptr);
    assert(sprite >= 0 && sprite < NUM_SPRITES);
    assert(camera != nullptr);
    assert(scale > 0);

    sprite_entry_t const* const entry = &(self->sprites[sprite]);

    if (entry->num_elements == 0) {
        return;
    }

    shaders_t* const shaders = rudyscung_get_shaders(self->rudyscung);
    shader_t* const shader = shaders_get(shaders, "main");
    shader_bind(shader);

    float camera_pos[NUM_AXES];
    camera_get_pos(camera, camera_pos);
    float camera_rot[NUM_ROT_AXES];
    camera_get_rot(camera, camera_rot);

    size_t window_size[2];
    window_get_size(rudyscung_get_window(self->rudyscung), window_size);

    camera_set_matrices(camera, window_size, shader);

    mat4 mat_model;
    glm_mat4_identity(mat_model);
    glm_translate(mat_model, (vec3) { pos[AXIS__X], pos[AXIS__Y], pos[AXIS__Z] });
    if (rotate[ROT_AXIS__Y]) glm_rotate(mat_model, -camera_rot[ROT_AXIS__Y], (vec3) { 0.0f, 1.0f, 0.0f });
    if (rotate[ROT_AXIS__X]) glm_rotate(mat_model, -camera_rot[ROT_AXIS__X], (vec3) { 1.0f, 0.0f, 0.0f });
    glm_scale(mat_model, (vec3) { scale, scale, scale });
    shader_put_uniform_mat4(shader, "model", mat_model);

    float delta[2] = {
        pos[AXIS__X] - camera_pos[AXIS__X],
        pos[AXIS__Z] - camera_pos[AXIS__Z]
    };

    float angle_y = atan2(delta[1], delta[0]) - rotation_offset;
    while (angle_y < -M_PI) {
        angle_y += 2 * M_PI;
    }
    while (angle_y > M_PI) {
        angle_y -= 2 * M_PI;
    }

    float q_pi = M_PI / 4;

    GLuint tex = 0;
    if (angle_y < -q_pi * 3 || angle_y > q_pi * 3) {
        tex = entry->tex[SPRITE_ANGLE__RIGHT];
    } else if (angle_y > -q_pi * 3 && angle_y < -q_pi) {
        tex = entry->tex[SPRITE_ANGLE__BACK];
    } else if (angle_y > -q_pi && angle_y < q_pi) {
        tex = entry->tex[SPRITE_ANGLE__LEFT];
    } else {
        tex = entry->tex[SPRITE_ANGLE__FRONT];
    }

    glBindTexture(GL_TEXTURE_2D, tex);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(entry->vao);
    glDrawArrays(GL_TRIANGLES, 0, entry->num_elements);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

static void sprite_new(sprites_t* const self, sprite_t const sprite, textures_t* const textures, tessellator_t* const tessellator, texture_name_t const texture_names[NUM_SPRITE_ANGLES], size_t size[2], float origin[2]) {
    assert(self != nullptr);
    assert(sprite >= 0 && sprite < NUM_SPRITES);
    assert(textures != nullptr);

    sprite_entry_t* const entry = &(self->sprites[sprite]);

    for (sprite_angle_t angle = 0; angle < NUM_SPRITE_ANGLES; angle++) {
        entry->tex[angle] = textures_get_texture(textures, texture_names[angle])->name;
    }
    memcpy(entry->size, size, sizeof(size_t) * 2);
    memcpy(entry->origin, origin, sizeof(float) * 2);
    entry->num_elements = 0;

    // Set up arrays
    glGenVertexArrays(1, &(entry->vao));
    assert(entry->vao > 0);
    glBindVertexArray(entry->vao);

    glGenBuffers(1, &(entry->vbo));
    assert(entry->vbo > 0);

    float origin_x = -entry->origin[0] * size[0];
    float origin_y = size[1] - entry->origin[1] * size[1];

    tessellator_bind(tessellator, entry->vao, entry->vbo, 0);

    tessellator_buffer_vct(tessellator, origin_x + size[0], origin_y + size[1], 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    tessellator_buffer_vct(tessellator, origin_x, origin_y, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    tessellator_buffer_vct(tessellator, origin_x + size[0], origin_y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

    tessellator_buffer_vct(tessellator, origin_x + size[0], origin_y + size[1], 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    tessellator_buffer_vct(tessellator, origin_x, origin_y + size[1], 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    tessellator_buffer_vct(tessellator, origin_x, origin_y, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

    entry->num_elements = tessellator_draw(tessellator);

    glBindVertexArray(0);
}