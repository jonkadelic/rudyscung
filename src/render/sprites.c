#include "./sprites.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "tessellator.h"
#include "textures.h"
#include "../rudyscung.h"
#include "./camera.h"
#include "../util.h"

typedef struct sprite_entry {
    GLuint tex;
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

static void sprite_new(sprites_t* const self, sprite_t const sprite, textures_t* const textures, tessellator_t* tessellator, texture_name_t const texture_name, size_t size[2], float origin[2]);

sprites_t* const sprites_new(rudyscung_t* const rudyscung) {
    assert(rudyscung != nullptr);

    sprites_t* self = malloc(sizeof(sprites_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;

    textures_t* const textures = rudyscung_get_textures(rudyscung);
    tessellator_t* const tessellator = tessellator_new();

    sprite_new(self, SPRITE__TREE, textures, tessellator, TEXTURE_NAME__SPRITE_TREE, (size_t[2]) { 256, 256 }, (float[2]) { 0.5f, 1.0f });
    sprite_new(self, SPRITE__MOB, textures, tessellator, TEXTURE_NAME__SPRITE_MOB, (size_t[2]) { 16, 32 }, (float[2]) { 0.5f, 1.0f });

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

void sprites_render(sprites_t const* const self, sprite_t const sprite, camera_t const* const camera, float const scale, float const pos[NUM_AXES], bool const rotate[NUM_ROT_AXES]) {
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

    float camera_rot[NUM_ROT_AXES];
    camera_get_rot(camera, camera_rot);

    size_t window_size[2];
    window_get_size(rudyscung_get_window(self->rudyscung), window_size);

    camera_set_matrices(camera, window_size, shader);

    mat4x4 mat_model;
    mat4x4_identity(mat_model);
    mat4x4_translate_in_place(mat_model, pos[AXIS__X], pos[AXIS__Y], pos[AXIS__Z]);
    if (rotate[ROT_AXIS__Y]) mat4x4_rotate(mat_model, mat_model, 0.0f, 1.0f, 0.0f, -camera_rot[ROT_AXIS__Y]);
    if (rotate[ROT_AXIS__X]) mat4x4_rotate(mat_model, mat_model, 1.0f, 0.0f, 0.0f, -camera_rot[ROT_AXIS__X]);
    mat4x4_scale_aniso(mat_model, mat_model, scale, scale, scale);
    shader_put_uniform_mat4x4(shader, "model", mat_model);

    glBindTexture(GL_TEXTURE_2D, entry->tex);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(entry->vao);
    glDrawArrays(GL_TRIANGLES, 0, entry->num_elements);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

static void sprite_new(sprites_t* const self, sprite_t const sprite, textures_t* const textures, tessellator_t* const tessellator, texture_name_t const texture_name, size_t size[2], float origin[2]) {
    assert(self != nullptr);
    assert(sprite >= 0 && sprite < NUM_SPRITES);
    assert(textures != nullptr);

    sprite_entry_t* const entry = &(self->sprites[sprite]);

    entry->tex = textures_get_texture(textures, texture_name)->name;
    memcpy(entry->size, size, sizeof(size_t) * 2);
    memcpy(entry->origin, origin, sizeof(float) * 2);
    entry->num_elements = 0;

    // Set up arrays
    glGenVertexArrays(1, &(entry->vao));
    assert(entry->vao > 0);
    glBindVertexArray(entry->vao);

    glGenBuffers(1, &(entry->vbo));
    assert(entry->vbo > 0);

    float origin_x = entry->origin[0];
    float origin_y = 1 - entry->origin[1];

    float half_width = size[0] / 2.0f;
    float half_height = size[1] / 2.0f;

    tessellator_bind(tessellator, entry->vao, entry->vbo, 0);

    tessellator_buffer_vct(tessellator, origin_x - 0.0f, origin_y + half_height, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    tessellator_buffer_vct(tessellator, origin_x - half_width, origin_y + 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
    tessellator_buffer_vct(tessellator, origin_x - 0.0f, origin_y + 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

    tessellator_buffer_vct(tessellator, origin_x - 0.0f, origin_y + half_height, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    tessellator_buffer_vct(tessellator, origin_x - half_width, origin_y + half_height, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    tessellator_buffer_vct(tessellator, origin_x - half_width, origin_y + 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f);

    entry->num_elements = tessellator_draw(tessellator);

    glBindVertexArray(0);
}