#include "./level_renderer.h"

#include <assert.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "../rudyscung.h"
#include "../world/chunk.h"
#include "../world/level.h"
#include "../util.h"
#include "./chunk_renderer.h"
#include "./tessellator.h"
#include "./shaders.h"
#include "./shader.h"
#include "./textures.h"
#include "camera.h"

#define CHUNK_COORD(x, y, z) (((y) * self->size_z * self->size_x) + ((z) * self->size_x) + (x))
#define TO_CHUNK_SPACE(tile_coord) ((tile_coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(chunk_coord) ((chunk_coord) * CHUNK_SIZE)

struct level_renderer {
    rudyscung_t* rudyscung;
    level_t const* level;
    size_chunks_t size_x;
    size_chunks_t size_y;
    size_chunks_t size_z;
    tessellator_t* tessellator;
    chunk_renderer_t** chunk_renderers;
    bool all_ready;
};

level_renderer_t* const level_renderer_new(rudyscung_t* const rudyscung, level_t const* const level) {
    assert(level != nullptr);

    level_renderer_t* const self = malloc(sizeof(level_renderer_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;
    self->level = level;

    size_chunks_t size[3];
    level_get_size(level, size);
    self->size_x = size[0];
    self->size_y = size[1];
    self->size_z = size[2];

    self->tessellator = tessellator_new();

    self->chunk_renderers = malloc(sizeof(chunk_renderer_t*) * self->size_y * self->size_z * self->size_x);
    assert(self->chunk_renderers != nullptr);

    for (size_chunks_t x = 0; x < self->size_x; x++) {
        for (size_chunks_t y = 0; y < self->size_y; y++) {
            for (size_chunks_t z = 0; z < self->size_z; z++) {
                chunk_t const* const chunk = level_get_chunk(level, x, y, z);
                self->chunk_renderers[CHUNK_COORD(x, y, z)] = chunk_renderer_new(self, chunk);
                assert(self->chunk_renderers[CHUNK_COORD(x, y, z)] != nullptr);
            }
        }
    }

    self->all_ready = false;

    return self;
}

void level_renderer_delete(level_renderer_t* const self) {
    assert(self != nullptr);

    for (size_chunks_t x = 0; x < self->size_x; x++) {
        for (size_chunks_t y = 0; y < self->size_y; y++) {
            for (size_chunks_t z = 0; z < self->size_z; z++) {
                chunk_renderer_delete(self->chunk_renderers[CHUNK_COORD(x, y, z)]);
                self->chunk_renderers[CHUNK_COORD(x, y, z)] = nullptr;
            }
        }
    }
    free(self->chunk_renderers);
    free(self);
}

void level_renderer_tick(level_renderer_t* const self) {
    assert(self != nullptr);

    if (!self->all_ready) {
        size_t remaining = 10;
        for (size_chunks_t i = 0; i < self->size_x * self->size_y * self->size_z; i++) {
            if (remaining == 0) {
                break;
            }

            chunk_renderer_t* const chunk_renderer = self->chunk_renderers[i];
            if (!chunk_renderer_is_ready(chunk_renderer)) {
                chunk_renderer_build(chunk_renderer, self->tessellator);
                remaining--;
            }
        }

        if (remaining == 10) {
            self->all_ready = true;
        }
    }
}

void level_renderer_draw(level_renderer_t const* const self, camera_t const* const camera) {
    assert(self != nullptr);

    shaders_t* const shaders = rudyscung_get_shaders(self->rudyscung);
    shader_t* const shader = shaders_get(shaders, "main");
    shader_bind(shader);

    float camera_pos[3];
    camera_get_pos(camera, camera_pos);
    float camera_rot[2];
    camera_get_rot(camera, camera_rot);

    mat4x4 mat_view;
    mat4x4_identity(mat_view);
    mat4x4_rotate(mat_view, mat_view, 1.0f, 0.0f, 0.0f, camera_rot[1]);
    mat4x4_rotate(mat_view, mat_view, 0.0f, 1.0f, 0.0f, camera_rot[0]);
    mat4x4_translate_in_place(mat_view, camera_pos[0], -camera_pos[1], camera_pos[2]);
    shader_put_uniform_mat4x4(shader, "view", mat_view);

    mat4x4 mat_proj;
    mat4x4_identity(mat_proj);
    // mat4x4_ortho(mat_proj, 0, 2.0f, 0, 2.0f, 0.1f, 100.0f);
    mat4x4_perspective(mat_proj, TO_RADIANS(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    shader_put_uniform_mat4x4(shader, "projection", mat_proj);

    mat4x4 mat_model;
    mat4x4_identity(mat_model);
    shader_put_uniform_mat4x4(shader, "model", mat_model);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    textures_t* const textures = rudyscung_get_textures(self->rudyscung);
    glBindTexture(GL_TEXTURE_2D, textures_get_texture(textures, TEXTURE_NAME__TERRAIN));

    for (size_chunks_t x = 0; x < self->size_x; x++) {
        for (size_chunks_t y = 0; y < self->size_y; y++) {
            for (size_chunks_t z = 0; z < self->size_z; z++) {
                chunk_renderer_t const* const chunk_renderer = self->chunk_renderers[CHUNK_COORD(x, y, z)];

                if (chunk_renderer_is_ready(chunk_renderer)) {
                    mat4x4_identity(mat_model);
                    mat4x4_translate_in_place(mat_model, TO_TILE_SPACE(x), TO_TILE_SPACE(y), TO_TILE_SPACE(z));
                    shader_put_uniform_mat4x4(shader, "model", mat_model);

                    chunk_renderer_draw(chunk_renderer);
                }
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

bool level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const x, size_t const y, size_t const z, side_t const side) {
    assert(self != nullptr);
    assert(x >= 0 && x < TO_TILE_SPACE(self->size_x));
    assert(y >= 0 && y < TO_TILE_SPACE(self->size_y));
    assert(z >= 0 && z < TO_TILE_SPACE(self->size_z));
    assert(side >= 0 && side < NUM_SIDES);

    if (
        (x == 0 && side == SIDE__NORTH) ||
        (x == TO_TILE_SPACE(self->size_x) - 1 && side == SIDE__SOUTH) ||
        (y == 0 && side == SIDE__BOTTOM) ||
        (z == 0 && side == SIDE__WEST) ||
        (z == TO_TILE_SPACE(self->size_z) - 1 && side == SIDE__EAST)
    ) {
        return true;
    }
    if (y == TO_TILE_SPACE(self->size_y) - 1 && side == SIDE__TOP) {
        return false;
    }

    int offsets[3];
    side_get_offsets(side, offsets);

    int tx = x + offsets[0];
    int ty = y + offsets[1];
    int tz = z + offsets[2];

    tile_shape_t tshape = level_get_tile_shape(self->level, tx, ty, tz);
    side_t tside = side_get_opposite(side);

    return tile_shape_can_side_occlude(tshape, tside);
}