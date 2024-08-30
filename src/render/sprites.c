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

typedef struct sprite_entry {
    texture_t** tex;
    size_t num_angles;
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

static void sprite_new(sprites_t* const self, sprite_t const sprite, textures_t* const textures, tessellator_t* tessellator, size_t const num_texture_paths, char const* const texture_paths[], size_t size[2], float origin[2]);

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
        1,
        (char const*[]) {
            "/sprite/tree.png"
        }, 
        (size_t[2]) { 64, 128 }, 
        (float[2]) { 0.5f, 1.0f }
    );
    sprite_new(self,
        SPRITE__MOB,
        textures,
        tessellator,
        4,
        (char const*[]) {
            "/sprite/mob/front.png",
            "/sprite/mob/right.png",
            "/sprite/mob/back.png",
            "/sprite/mob/left.png"
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
        sprite_entry_t* const sprite_entry = &(self->sprites[sprite]);
        glDeleteBuffers(1, &(sprite_entry->vbo));
        glDeleteVertexArrays(1, &(sprite_entry->vao));
        for (size_t i = 0; i < sprite_entry->num_angles; i++) {
            texture_delete(sprite_entry->tex[i]);
        }
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

    float angle_y = (atan2(delta[1], delta[0]) - rotation_offset) + M_PI; // Range = 0 - 2pi

    GLuint tex;
    if (entry->num_angles == 1) {
        tex = entry->tex[0]->name;
    } else {
        // front = 3/2pi
        float const base = M_PI * 1.5f;
        float const increment = (2.0f * M_PI) / entry->num_angles;
        float const half_increment = increment / 2.0f;

        angle_y -= base - half_increment;
        while (angle_y < 0.0f) {
            angle_y += 2 * M_PI;
        }
        while (angle_y > 2 * M_PI) {
            angle_y -= 2 * M_PI;
        }
        assert(angle_y >= 0.0f && angle_y < 2.0f * M_PI);
        size_t const tex_index = (size_t) (angle_y / increment);

        tex = entry->tex[tex_index]->name;
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

static void sprite_new(sprites_t* const self, sprite_t const sprite, textures_t* const textures, tessellator_t* tessellator, size_t const num_texture_paths, char const* const texture_paths[], size_t size[2], float origin[2]) {
    assert(self != nullptr);
    assert(sprite >= 0 && sprite < NUM_SPRITES);
    assert(textures != nullptr);

    sprite_entry_t* const entry = &(self->sprites[sprite]);
    entry->tex = malloc(sizeof(texture_t*) * num_texture_paths);

    for (size_t i = 0; i < num_texture_paths; i++) {
        entry->tex[i] = textures_load_texture(textures, texture_paths[i]);
    }
    memcpy(entry->size, size, sizeof(size_t) * 2);
    memcpy(entry->origin, origin, sizeof(float) * 2);
    entry->num_elements = 0;
    entry->num_angles = num_texture_paths;

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