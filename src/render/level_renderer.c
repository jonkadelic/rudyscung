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
#include "./sprites.h"

#define CHUNK_INDEX(x, y, z) (((y) * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X]) + ((z) * self->level_slice.size[AXIS__X]) + (x))
#define TO_CHUNK_SPACE(tile_coord) ((tile_coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(chunk_coord) ((chunk_coord) * CHUNK_SIZE)

struct level_renderer {
    rudyscung_t* rudyscung;
    level_t* level;
    tessellator_t* tessellator;
    sprites_t* sprites;
    chunk_renderer_t** chunk_renderers;
    level_slice_t level_slice;
    bool all_ready;
};

static void delete_chunk_renderers(level_renderer_t* const self);
static void reload_chunk_renderers(level_renderer_t* const self);

level_renderer_t* const level_renderer_new(rudyscung_t* const rudyscung, level_t* const level) {
    assert(level != nullptr);

    level_renderer_t* const self = malloc(sizeof(level_renderer_t));
    assert(self != nullptr);

    self->rudyscung = rudyscung;
    self->level = level;

    size_chunks_t size[NUM_AXES];
    level_get_size(level, size);
    memcpy(self->level_slice.size, size, sizeof(size_chunks_t) * NUM_AXES);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        self->level_slice.pos[a] = 0;
    }

    self->tessellator = tessellator_new();
    self->sprites = sprites_new(rudyscung);

    self->chunk_renderers = nullptr;

    return self;
}

void level_renderer_delete(level_renderer_t* const self) {
    assert(self != nullptr);

    tessellator_delete(self->tessellator);

    delete_chunk_renderers(self);

    sprites_delete(self->sprites);
    
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

    if (memcmp(slice, &(self->level_slice), sizeof(level_slice_t)) == 0) {
        return;
    }
    
    level_slice_t old_slice;
    memcpy(&(old_slice), &(self->level_slice), sizeof(level_slice_t));
    float f_old_slice_pos[NUM_AXES];
    float f_slice_pos[NUM_AXES];
    float f_old_slice_pos_max[NUM_AXES];
    float f_slice_pos_max[NUM_AXES];
    for (axis_t a = 0; a < NUM_AXES; a++) {
        f_old_slice_pos[a] = (float) old_slice.pos[a];
        f_old_slice_pos_max[a] = (float) old_slice.pos[a] + (float) old_slice.size[a];
        f_slice_pos[a] = (float) slice->pos[a];
        f_slice_pos_max[a] = (float) slice->pos[a] + (float) slice->size[a];
    }
    aabb_t* const old_aabb = aabb_new(f_old_slice_pos, f_old_slice_pos_max);
    aabb_t* const new_aabb = aabb_new(f_slice_pos, f_slice_pos_max);

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
    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (slice->pos[a] < old_slice.pos[a]) {
            overlap.pos[a] = old_slice.pos[a];
            overlap.size[a] = ((old_slice.pos[a] + old_slice.size[a]) - slice->pos[a]) - ((old_slice.pos[a] - slice->pos[a]) + ((old_slice.pos[a] + old_slice.size[a]) - (slice->pos[a] + slice->size[a])));
        } else {
            overlap.pos[a] = slice->pos[a];
            overlap.size[a] = ((slice->pos[a] + slice->size[a]) - old_slice.pos[a]) - ((slice->pos[a] - old_slice.pos[a]) + ((slice->pos[a] + slice->size[a]) - (old_slice.pos[a] + old_slice.size[a])));
        }

        if (overlap.size[a] > slice->size[a]) {
            overlap.size[a] = slice->size[a];
        }
        if (overlap.size[a] > old_slice.size[a]) {
            overlap.size[a] = old_slice.size[a];
        }
    }

    size_t const chunks_area = slice->size[AXIS__Y] * slice->size[AXIS__Z] * slice->size[AXIS__X];
    chunk_renderer_t** new_chunk_renderers = malloc(sizeof(chunk_renderer_t*) * chunks_area);
    assert(new_chunk_renderers != nullptr);
    for (size_t i = 0; i < chunks_area; i++) {
        new_chunk_renderers[i] = nullptr;
    }

    for (size_chunks_t x = 0; x < overlap.size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < overlap.size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < overlap.size[AXIS__Z]; z++) {
                size_t old_index = ((((overlap.pos[AXIS__Y] - old_slice.pos[AXIS__Y]) + y) * old_slice.size[AXIS__Z] * old_slice.size[AXIS__X]) + (((overlap.pos[AXIS__Z] - old_slice.pos[AXIS__Z]) + z) * old_slice.size[AXIS__X]) + ((overlap.pos[AXIS__X] - old_slice.pos[AXIS__X]) + x));
                size_t new_index = ((((overlap.pos[AXIS__Y] - slice->pos[AXIS__Y]) + y) * slice->size[AXIS__Z] * slice->size[AXIS__X]) + (((overlap.pos[AXIS__Z] - slice->pos[AXIS__Z]) + z) * slice->size[AXIS__X]) + ((overlap.pos[AXIS__X] - slice->pos[AXIS__X]) + x));

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
        size_t const chunks_area = self->level_slice.size[AXIS__Y] * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X];
        for (size_chunks_t i = 0; i < chunks_area; i++) {
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
    assert(self->chunk_renderers != nullptr);

    shaders_t* const shaders = rudyscung_get_shaders(self->rudyscung);
    shader_t* const shader = shaders_get(shaders, "main");
    shader_bind(shader);

    float camera_pos[NUM_AXES];
    camera_get_pos(camera, camera_pos);
    float camera_rot[NUM_ROT_AXES];
    camera_get_rot(camera, camera_rot);

    size_t window_size[2];
    window_get_size(rudyscung_get_window(self->rudyscung), window_size);

    mat4x4 mat_view;
    mat4x4_identity(mat_view);
    mat4x4_rotate(mat_view, mat_view, 1.0f, 0.0f, 0.0f, camera_rot[ROT_AXIS__X]);
    mat4x4_rotate(mat_view, mat_view, 0.0f, 1.0f, 0.0f, camera_rot[ROT_AXIS__Y]);
    mat4x4_translate_in_place(mat_view, -camera_pos[AXIS__X], -camera_pos[AXIS__Y], -camera_pos[AXIS__Z]);
    shader_put_uniform_mat4x4(shader, "view", mat_view);

    mat4x4 mat_proj;
    mat4x4_identity(mat_proj);
    // mat4x4_ortho(mat_proj, 0, 2.0f, 0, 2.0f, 0.1f, 100.0f);
    mat4x4_perspective(mat_proj, TO_RADIANS(65.0f), (float) window_size[0] / window_size[1], 0.1f, 1000.0f);
    shader_put_uniform_mat4x4(shader, "projection", mat_proj);

    mat4x4 mat_model;
    mat4x4_identity(mat_model);
    shader_put_uniform_mat4x4(shader, "model", mat_model);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    textures_t* const textures = rudyscung_get_textures(self->rudyscung);
    glBindTexture(GL_TEXTURE_2D, textures_get_texture(textures, TEXTURE_NAME__TERRAIN));

    for (size_chunks_t x = 0; x < self->level_slice.size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < self->level_slice.size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < self->level_slice.size[AXIS__Z]; z++) {
                chunk_renderer_t const* const chunk_renderer = self->chunk_renderers[CHUNK_INDEX(x, y, z)];

                if (chunk_renderer != nullptr) {
                    if (chunk_renderer_is_ready(chunk_renderer)) {
                        mat4x4_identity(mat_model);
                        mat4x4_translate_in_place(mat_model, TO_TILE_SPACE(self->level_slice.pos[AXIS__X]) + TO_TILE_SPACE(x), TO_TILE_SPACE(self->level_slice.pos[AXIS__Y]) + TO_TILE_SPACE(y), TO_TILE_SPACE(self->level_slice.pos[AXIS__Z]) + TO_TILE_SPACE(z));
                        shader_put_uniform_mat4x4(shader, "model", mat_model);

                        chunk_renderer_draw(chunk_renderer);
                    }
                }
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    ecs_t* ecs = level_get_ecs(self->level);

    for (size_t i = 0; i < NUM_TREES; i++) {
        entity_t tree = level_get_tree(self->level, i);
        ecs_component_pos_t const* const tree_pos = ecs_get_component_data(ecs, tree, ECS_COMPONENT__POS);
        ecs_component_sprite_t const* const tree_sprite = ecs_get_component_data(ecs, tree, ECS_COMPONENT__SPRITE);

        sprites_render(self->sprites, tree_sprite->sprite, camera, 10.0f, tree_pos->pos, (bool[NUM_ROT_AXES]) { true, false });
    }

}

bool level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const pos[NUM_AXES], side_t const side) {
    assert(self != nullptr);
    assert(side >= 0 && side < NUM_SIDES);

    size_chunks_t level_size[NUM_AXES];
    level_get_size(self->level, level_size);

    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(level_size[a]));
    }

    if (
        (pos[AXIS__X] == 0 && side == SIDE__NORTH) ||
        (pos[AXIS__X] == TO_TILE_SPACE(level_size[AXIS__X]) - 1 && side == SIDE__SOUTH) ||
        (pos[AXIS__Y] == 0 && side == SIDE__BOTTOM) ||
        (pos[AXIS__Z] == 0 && side == SIDE__WEST) ||
        (pos[AXIS__Z] == TO_TILE_SPACE(level_size[AXIS__Z]) - 1 && side == SIDE__EAST)
    ) {
        return true;
    }
    if (pos[AXIS__Y] == TO_TILE_SPACE(level_size[AXIS__Y]) - 1 && side == SIDE__TOP) {
        return false;
    }

    int offsets[NUM_AXES];
    side_get_offsets(side, offsets);

    size_t t_pos[NUM_AXES];
    for (axis_t a = 0; a < NUM_AXES; a++) {
        t_pos[a] = pos[a] + offsets[a];
    }

    tile_shape_t tshape = level_get_tile_shape(self->level, t_pos);
    side_t tside = side_get_opposite(side);

    return tile_shape_can_side_occlude(tshape, tside);
}

static void delete_chunk_renderers(level_renderer_t* const self) {
    assert(self != nullptr);

    size_t const chunks_area = self->level_slice.size[AXIS__Y] * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X];
    if (self->chunk_renderers != nullptr) {
        for (size_t i = 0; i < chunks_area; i++) {
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

    size_t const chunks_area = self->level_slice.size[AXIS__Y] * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X];
    if (self->chunk_renderers == nullptr) {
        self->chunk_renderers = malloc(sizeof(chunk_renderer_t*) * chunks_area);
        assert(self->chunk_renderers != nullptr);

        for (size_t i = 0; i < chunks_area; i++) {
            self->chunk_renderers[i] = nullptr;
        }
    }

    size_chunks_t level_size[NUM_AXES];
    level_get_size(self->level, level_size);

    for (size_chunks_t x = 0; x < self->level_slice.size[AXIS__X] && (self->level_slice.pos[AXIS__X] + x) < level_size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < self->level_slice.size[AXIS__Y] && (self->level_slice.pos[AXIS__Y] + y) < level_size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < self->level_slice.size[AXIS__Z] && (self->level_slice.pos[AXIS__Z] + z) < level_size[AXIS__Z]; z++) {
                if (self->chunk_renderers[CHUNK_INDEX(x, y, z)] == nullptr) {
                    chunk_t const* const chunk = level_get_chunk(self->level, (size_chunks_t[NUM_AXES]) { self->level_slice.pos[AXIS__X] + x, self->level_slice.pos[AXIS__Y] + y, self->level_slice.pos[AXIS__Z] + z });
                    self->chunk_renderers[CHUNK_INDEX(x, y, z)] = chunk_renderer_new(self, chunk);
                }
            }
        }
    }

    self->all_ready = false;
}