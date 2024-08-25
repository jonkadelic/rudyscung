#include "./level_renderer.h"

#include <assert.h>
#include <stdlib.h>

#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "../phys/aabb.h"
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

#define CHUNK_INDEX(x, y, z) (((y) * self->level_slice.size_z * self->level_slice.size_x) + ((z) * self->level_slice.size_x) + (x))
#define TO_CHUNK_SPACE(tile_coord) ((tile_coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(chunk_coord) ((chunk_coord) * CHUNK_SIZE)

struct level_renderer {
    rudyscung_t* rudyscung;
    level_t const* level;
    tessellator_t* tessellator;
    chunk_renderer_t** chunk_renderers;
    level_slice_t level_slice;
    bool all_ready;
};

static void delete_chunk_renderers(level_renderer_t* const self);
static void reload_chunk_renderers(level_renderer_t* const self);

level_renderer_t* const level_renderer_new(rudyscung_t* const rudyscung, level_t const* const level) {
    assert(level != nullptr);

    level_renderer_t* const self = malloc(sizeof(level_renderer_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;
    self->level = level;

    size_chunks_t size[3];
    level_get_size(level, size);
    self->level_slice.size_x = size[0];
    self->level_slice.size_y = size[1];
    self->level_slice.size_z = size[2];
    self->level_slice.x = 0;
    self->level_slice.y = 0;
    self->level_slice.z = 0;

    self->tessellator = tessellator_new();

    self->chunk_renderers = nullptr;

    return self;
}

void level_renderer_delete(level_renderer_t* const self) {
    assert(self != nullptr);

    delete_chunk_renderers(self);
    
    free(self);
}

void level_renderer_slice(level_renderer_t* const self, level_slice_t const* const slice) {
    assert(self != nullptr);
    assert(slice != nullptr);

    if (self->chunk_renderers == nullptr) {
        memcpy(&(self->level_slice), slice, sizeof(level_slice_t));
        reload_chunk_renderers(self);
        return;
    }

    if (slice->size_x == self->level_slice.size_x && slice->size_y == self->level_slice.size_y && slice->size_z == self->level_slice.size_z &&
        slice->x == self->level_slice.x && slice->y == self->level_slice.y && slice->z == self->level_slice.z
    ) {
        return;
    }

    level_slice_t old_slice;
    memcpy(&(old_slice), &(self->level_slice), sizeof(level_slice_t));
    aabb_t* const old_aabb = aabb_new(old_slice.x, old_slice.y, old_slice.z, old_slice.x + old_slice.size_x, old_slice.y + old_slice.size_y, old_slice.z + old_slice.size_z);
    aabb_t* const new_aabb = aabb_new(slice->x, slice->y, slice->z, slice->x + slice->size_x, slice->y + slice->size_y, slice->z + slice->size_z);

    bool does_overlap = aabb_test_aabb_overlap(old_aabb, new_aabb);

    aabb_delete(old_aabb);
    aabb_delete(new_aabb);

    if (!does_overlap) {
        delete_chunk_renderers(self);
        memcpy(&(self->level_slice), slice, sizeof(level_slice_t));
        reload_chunk_renderers(self);
        return;
    }

    level_slice_t overlap;
    if (slice->x < old_slice.x) {
        overlap.x = old_slice.x;
        overlap.size_x = ((old_slice.x + old_slice.size_x) - slice->x) - ((old_slice.x - slice->x) + ((old_slice.x + old_slice.size_x) - (slice->x + slice->size_x)));
    } else {
        overlap.x = slice->x;
        overlap.size_x = ((slice->x + slice->size_x) - old_slice.x) - ((slice->x - old_slice.x) + ((slice->x + slice->size_x) - (old_slice.x + old_slice.size_x)));
    }
    if (slice->y < old_slice.y) {
        overlap.y = old_slice.y;
        overlap.size_y = ((old_slice.y + old_slice.size_y) - slice->y) - ((old_slice.y - slice->y) + ((old_slice.y + old_slice.size_y) - (slice->y + slice->size_y)));
    } else {
        overlap.y = slice->y;
        overlap.size_y = ((slice->y + slice->size_y) - old_slice.y) - ((slice->y - old_slice.y) + ((slice->y + slice->size_y) - (old_slice.y + old_slice.size_y)));
    }
    if (slice->z < old_slice.z) {
        overlap.z = old_slice.z;
        overlap.size_z = ((old_slice.z + old_slice.size_z) - slice->z) - ((old_slice.z - slice->z) + ((old_slice.z + old_slice.size_z) - (slice->z + slice->size_z)));
    } else {
        overlap.z = slice->z;
        overlap.size_z = ((slice->z + slice->size_z) - old_slice.z) - ((slice->z - old_slice.z) + ((slice->z + slice->size_z) - (old_slice.z + old_slice.size_z)));
    }
    if (overlap.size_x > slice->size_x) {
        overlap.size_x = slice->size_x;
    }
    if (overlap.size_x > old_slice.size_x) {
        overlap.size_x = old_slice.size_x;
    }
    if (overlap.size_y > slice->size_y) {
        overlap.size_y = slice->size_y;
    }
    if (overlap.size_y > old_slice.size_y) {
        overlap.size_y = old_slice.size_y;
    }
    if (overlap.size_z > slice->size_z) {
        overlap.size_z = slice->size_z;
    }
    if (overlap.size_z > old_slice.size_z) {
        overlap.size_z = old_slice.size_z;
    }

    chunk_renderer_t** new_chunk_renderers = malloc(sizeof(chunk_renderer_t*) * slice->size_y * slice->size_z * slice->size_x);
    assert(new_chunk_renderers != nullptr);
    for (size_t i = 0; i < (slice->size_y * slice->size_z * slice->size_x); i++) {
        new_chunk_renderers[i] = nullptr;
    }

    for (size_chunks_t x = 0; x < overlap.size_x; x++) {
        for (size_chunks_t y = 0; y < overlap.size_y; y++) {
            for (size_chunks_t z = 0; z < overlap.size_z; z++) {
                size_t old_index = ((((overlap.y - old_slice.y) + y) * old_slice.size_z * old_slice.size_x) + (((overlap.z - old_slice.z) + z) * old_slice.size_x) + ((overlap.x - old_slice.x) + x));
                size_t new_index = ((((overlap.y - slice->y) + y) * slice->size_z * slice->size_x) + (((overlap.z - slice->z) + z) * slice->size_x) + ((overlap.x - slice->x) + x));

                new_chunk_renderers[new_index] = self->chunk_renderers[old_index];
                self->chunk_renderers[old_index] = nullptr;
            }
        }
    }

    delete_chunk_renderers(self);

    memcpy(&(self->level_slice), slice, sizeof(level_slice_t));

    self->chunk_renderers = new_chunk_renderers;

    reload_chunk_renderers(self);
}

void level_renderer_tick(level_renderer_t* const self) {
    assert(self != nullptr);

    if (!self->all_ready) {
        size_t remaining = 20;
        for (size_chunks_t i = 0; i < self->level_slice.size_x * self->level_slice.size_y * self->level_slice.size_z; i++) {
            if (remaining == 0) {
                break;
            }

            chunk_renderer_t* const chunk_renderer = self->chunk_renderers[i];
            if (chunk_renderer != nullptr) {
                if (!chunk_renderer_is_ready(chunk_renderer)) {
                    chunk_renderer_build(chunk_renderer, self->tessellator);
                    remaining--;
                }
            }
        }

        if (remaining == 20) {
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

    size_t window_size[2];
    window_get_size(rudyscung_get_window(self->rudyscung), window_size);

    mat4x4 mat_view;
    mat4x4_identity(mat_view);
    mat4x4_rotate(mat_view, mat_view, 1.0f, 0.0f, 0.0f, camera_rot[1]);
    mat4x4_rotate(mat_view, mat_view, 0.0f, 1.0f, 0.0f, camera_rot[0]);
    mat4x4_translate_in_place(mat_view, -camera_pos[0], -camera_pos[1], -camera_pos[2]);
    shader_put_uniform_mat4x4(shader, "view", mat_view);

    mat4x4 mat_proj;
    mat4x4_identity(mat_proj);
    // mat4x4_ortho(mat_proj, 0, 2.0f, 0, 2.0f, 0.1f, 100.0f);
    mat4x4_perspective(mat_proj, TO_RADIANS(45.0f), (float) window_size[0] / window_size[1], 0.1f, 1000.0f);
    shader_put_uniform_mat4x4(shader, "projection", mat_proj);

    mat4x4 mat_model;
    mat4x4_identity(mat_model);
    shader_put_uniform_mat4x4(shader, "model", mat_model);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    textures_t* const textures = rudyscung_get_textures(self->rudyscung);
    glBindTexture(GL_TEXTURE_2D, textures_get_texture(textures, TEXTURE_NAME__TERRAIN));

    for (size_chunks_t x = 0; x < self->level_slice.size_x; x++) {
        for (size_chunks_t y = 0; y < self->level_slice.size_y; y++) {
            for (size_chunks_t z = 0; z < self->level_slice.size_z; z++) {
                chunk_renderer_t const* const chunk_renderer = self->chunk_renderers[CHUNK_INDEX(x, y, z)];

                if (chunk_renderer != nullptr) {
                    if (chunk_renderer_is_ready(chunk_renderer)) {
                        mat4x4_identity(mat_model);
                        mat4x4_translate_in_place(mat_model, TO_TILE_SPACE(self->level_slice.x) + TO_TILE_SPACE(x), TO_TILE_SPACE(self->level_slice.y) + TO_TILE_SPACE(y), TO_TILE_SPACE(self->level_slice.z) + TO_TILE_SPACE(z));
                        shader_put_uniform_mat4x4(shader, "model", mat_model);

                        chunk_renderer_draw(chunk_renderer);
                    }
                }
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

bool level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const x, size_t const y, size_t const z, side_t const side) {
    assert(self != nullptr);
    assert(side >= 0 && side < NUM_SIDES);

    size_chunks_t level_size[3];
    level_get_size(self->level, level_size);

    assert(x >= 0 && x < TO_TILE_SPACE(level_size[0]));
    assert(y >= 0 && y < TO_TILE_SPACE(level_size[1]));
    assert(z >= 0 && z < TO_TILE_SPACE(level_size[2]));

    if (
        (x == 0 && side == SIDE__NORTH) ||
        (x == TO_TILE_SPACE(level_size[0]) - 1 && side == SIDE__SOUTH) ||
        (y == 0 && side == SIDE__BOTTOM) ||
        (z == 0 && side == SIDE__WEST) ||
        (z == TO_TILE_SPACE(level_size[2]) - 1 && side == SIDE__EAST)
    ) {
        return true;
    }
    if (y == TO_TILE_SPACE(level_size[1]) - 1 && side == SIDE__TOP) {
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

static void delete_chunk_renderers(level_renderer_t* const self) {
    assert(self != nullptr);

    if (self->chunk_renderers != nullptr) {
        for (size_t i = 0; i < (self->level_slice.size_y * self->level_slice.size_z * self->level_slice.size_x); i++) {
            if (self->chunk_renderers[i] != nullptr) {
                chunk_renderer_delete(self->chunk_renderers[i]);
                self->chunk_renderers[i] = nullptr;
            }
        }
        
        free(self->chunk_renderers);
        self->chunk_renderers = nullptr;
    }
}

static void reload_chunk_renderers(level_renderer_t* const self) {
    assert(self != nullptr);

    if (self->chunk_renderers == nullptr) {
        self->chunk_renderers = malloc(sizeof(chunk_renderer_t*) * self->level_slice.size_y * self->level_slice.size_z * self->level_slice.size_x);
        assert(self->chunk_renderers != nullptr);

        for (size_t i = 0; i < (self->level_slice.size_y * self->level_slice.size_z * self->level_slice.size_x); i++) {
            self->chunk_renderers[i] = nullptr;
        }
    }

    size_chunks_t level_size[3];
    level_get_size(self->level, level_size);

    for (size_chunks_t x = 0; x < self->level_slice.size_x && (self->level_slice.x + x) < level_size[0]; x++) {
        for (size_chunks_t y = 0; y < self->level_slice.size_y && (self->level_slice.y + y) < level_size[1]; y++) {
            for (size_chunks_t z = 0; z < self->level_slice.size_z && (self->level_slice.z + z) < level_size[2]; z++) {
                if (self->chunk_renderers[CHUNK_INDEX(x, y, z)] == nullptr) {
                    chunk_t const* const chunk = level_get_chunk(self->level, self->level_slice.x + x, self->level_slice.y + y, self->level_slice.z + z);
                    self->chunk_renderers[CHUNK_INDEX(x, y, z)] = chunk_renderer_new(self, chunk);
                }
            }
        }
    }

    self->all_ready = false;
}