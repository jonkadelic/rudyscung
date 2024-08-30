#include "./level_renderer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/cglm.h>

#include "src/render/gl.h"
#include "src/world/entity/ecs.h"
#include "src/world/side.h"
#include "src/phys/aabb.h"
#include "src/client/client.h"
#include "src/world/chunk.h"
#include "src/world/level.h"
#include "src/util/util.h"
#include "src/render/chunk_renderer.h"
#include "src/render/tessellator.h"
#include "src/render/shaders.h"
#include "src/render/shader.h"
#include "src/render/textures.h"
#include "src/render/camera.h"
#include "src/render/sprites.h"
#include "src/phys/raycast.h"

#define CHUNK_INDEX(x, y, z) (((y) * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X]) + ((z) * self->level_slice.size[AXIS__X]) + (x))
#define TO_CHUNK_SPACE(tile_coord) ((tile_coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(chunk_coord) ((chunk_coord) * CHUNK_SIZE)

struct level_renderer {
    client_t* client;
    tessellator_t* tessellator;
    sprites_t* sprites;
    chunk_renderer_t** chunk_renderers;
    level_slice_t level_slice;
};

static void delete_chunk_renderers(level_renderer_t* const self);
static void reload_chunk_renderers(level_renderer_t* const self);

level_renderer_t* const level_renderer_new(client_t* const client) {
    assert(client != nullptr);

    level_renderer_t* const self = malloc(sizeof(level_renderer_t));
    assert(self != nullptr);

    self->client = client;

    self->tessellator = tessellator_new();
    self->sprites = sprites_new(client);

    level_renderer_level_changed(self);

    return self;
}

void level_renderer_delete(level_renderer_t* const self) {
    assert(self != nullptr);

    tessellator_delete(self->tessellator);

    delete_chunk_renderers(self);

    sprites_delete(self->sprites);
    
    free(self);
}

void level_renderer_level_changed(level_renderer_t* const self) {
    assert(self != nullptr);

    delete_chunk_renderers(self);

    level_t* const level = client_get_level(self->client);
    size_chunks_t size[NUM_AXES];
    level_get_size(level, size);
    memcpy(self->level_slice.size, size, sizeof(size_chunks_t) * NUM_AXES);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        self->level_slice.pos[a] = 0;
    }

    reload_chunk_renderers(self);
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

    level_t* const level = client_get_level(self->client);

    size_t remaining = 20;
    size_t const chunks_area = self->level_slice.size[AXIS__Y] * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X];
    for (size_chunks_t i = 0; i < chunks_area; i++) {
        if (remaining == 0) {
            break;
        }

        chunk_renderer_t* const chunk_renderer = self->chunk_renderers[i];
        if (chunk_renderer != nullptr) {
            chunk_t const* const chunk = chunk_renderer_get_chunk(chunk_renderer);
            size_chunks_t pos[NUM_AXES];
            chunk_get_pos(chunk, pos);

            if (!chunk_renderer_is_ready(chunk_renderer) || level_is_chunk_dirty(level, pos)) {
                chunk_renderer_build(chunk_renderer, self->tessellator);
                remaining--;
            }
        }
    }
}

void level_renderer_draw(level_renderer_t* const self, camera_t* const camera, float const partial_tick) {
    assert(self != nullptr);
    assert(self->chunk_renderers != nullptr);

    level_t* const level = client_get_level(self->client);

    shaders_t* const shaders = client_get_shaders(self->client);
    shader_t* const shader = shaders_get(shaders, "main");
    shader_bind(shader);

    size_t window_size[2];
    window_get_size(client_get_window(self->client), window_size);

    float camera_pos[NUM_AXES];
    camera_get_pos(camera, camera_pos);

    camera_set_matrices(camera, window_size, shader);

    mat4 mat_model;
    glm_mat4_identity(mat_model);
    shader_put_uniform_mat4(shader, "model", mat_model);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    textures_t* const textures = client_get_textures(self->client);
    glBindTexture(GL_TEXTURE_2D, textures_get_texture(textures, TEXTURE_NAME__TERRAIN)->name);

    for (size_chunks_t x = 0; x < self->level_slice.size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < self->level_slice.size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < self->level_slice.size[AXIS__Z]; z++) {
                chunk_renderer_t const* const chunk_renderer = self->chunk_renderers[CHUNK_INDEX(x, y, z)];

                if (chunk_renderer != nullptr) {
                    if (chunk_renderer_is_ready(chunk_renderer)) {
                        glm_mat4_identity(mat_model);
                        glm_translate(mat_model, (vec3) {
                            TO_TILE_SPACE(self->level_slice.pos[AXIS__X]) + TO_TILE_SPACE(x),
                            TO_TILE_SPACE(self->level_slice.pos[AXIS__Y]) + TO_TILE_SPACE(y),
                            TO_TILE_SPACE(self->level_slice.pos[AXIS__Z]) + TO_TILE_SPACE(z)
                        });
                        shader_put_uniform_mat4(shader, "model", mat_model);

                        chunk_renderer_draw(chunk_renderer);
                    }
                }
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    ecs_t* ecs = level_get_ecs(level);
    entity_t const highest_entity_id = ecs_get_highest_entity_id(ecs);

    for (entity_t entity = 0; entity <= highest_entity_id; entity++) {
        if (ecs_does_entity_exist(ecs, entity)) {
            if (ecs_has_component(ecs, entity, ECS_COMPONENT__POS) && ecs_has_component(ecs, entity, ECS_COMPONENT__SPRITE)) {
                ecs_component_pos_t const* const entity_pos = ecs_get_component_data(ecs, entity, ECS_COMPONENT__POS);
                ecs_component_sprite_t const* const entity_sprite = ecs_get_component_data(ecs, entity, ECS_COMPONENT__SPRITE);
                float rotation_offset = 0.0f;
                if (ecs_has_component(ecs, entity, ECS_COMPONENT__ROT)) {
                    ecs_component_rot_t const* const entity_rot = ecs_get_component_data(ecs, entity, ECS_COMPONENT__ROT);
                    rotation_offset = entity_rot->rot[ROT_AXIS__Y];
                }

                float distances[NUM_AXES] = VEC_SUB_INIT(entity_pos->pos, camera_pos);
                float distance_sq = (distances[AXIS__X] * distances[AXIS__X]) + (distances[AXIS__Y] * distances[AXIS__Y]) + (distances[AXIS__Z] * distances[AXIS__Z]);

                if (distance_sq < (24 * 24 * 24)) {
                    float pos[NUM_AXES];
                    if (ecs_has_component(ecs, entity, ECS_COMPONENT__VEL)) {
                        pos[AXIS__X] = lerp(entity_pos->pos_o[AXIS__X], entity_pos->pos[AXIS__X], partial_tick);
                        pos[AXIS__Y] = lerp(entity_pos->pos_o[AXIS__Y], entity_pos->pos[AXIS__Y], partial_tick);
                        pos[AXIS__Z] = lerp(entity_pos->pos_o[AXIS__Z], entity_pos->pos[AXIS__Z], partial_tick);
                    } else {
                        memcpy(pos, entity_pos->pos, sizeof(float) * NUM_AXES);
                    }
                    sprites_render(self->sprites, entity_sprite->sprite, camera, entity_sprite->scale, pos, rotation_offset, (bool[NUM_ROT_AXES]) { true, false });
                }
            }
        }
    }

    // Raycast and draw sprite
    ecs_component_pos_t const* const player_pos = ecs_get_component_data(ecs, client_get_player(self->client), ECS_COMPONENT__POS);
    ecs_component_rot_t const* const player_rot = ecs_get_component_data(ecs, client_get_player(self->client), ECS_COMPONENT__ROT);

    raycast_t raycast;
    raycast_cast_in_level(&raycast, level, player_pos->pos, player_rot->rot);
    if (raycast.hit) {
        sprites_render(self->sprites, SPRITE__TREE, camera, 0.0125f, raycast.pos, 0.0f, (bool[NUM_ROT_AXES]) { true, true });
    }
}

bool const level_renderer_is_tile_side_occluded(level_renderer_t const* const self, size_t const pos[NUM_AXES], side_t const side) {
    assert(self != nullptr);
    assert(side >= 0 && side < NUM_SIDES);

    level_t* const level = client_get_level(self->client);

    size_chunks_t level_size[NUM_AXES];
    level_get_size(level, level_size);

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

    tile_shape_t tshape = level_get_tile_shape(level, t_pos);
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

    level_t* const level = client_get_level(self->client);

    size_t const chunks_area = self->level_slice.size[AXIS__Y] * self->level_slice.size[AXIS__Z] * self->level_slice.size[AXIS__X];
    if (self->chunk_renderers == nullptr) {
        self->chunk_renderers = malloc(sizeof(chunk_renderer_t*) * chunks_area);
        assert(self->chunk_renderers != nullptr);

        for (size_t i = 0; i < chunks_area; i++) {
            self->chunk_renderers[i] = nullptr;
        }
    }

    size_chunks_t level_size[NUM_AXES];
    level_get_size(level, level_size);

    for (size_chunks_t x = 0; x < self->level_slice.size[AXIS__X] && (self->level_slice.pos[AXIS__X] + x) < level_size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < self->level_slice.size[AXIS__Y] && (self->level_slice.pos[AXIS__Y] + y) < level_size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < self->level_slice.size[AXIS__Z] && (self->level_slice.pos[AXIS__Z] + z) < level_size[AXIS__Z]; z++) {
                if (self->chunk_renderers[CHUNK_INDEX(x, y, z)] == nullptr) {
                    chunk_t const* const chunk = level_get_chunk(level, (size_chunks_t[NUM_AXES]) { self->level_slice.pos[AXIS__X] + x, self->level_slice.pos[AXIS__Y] + y, self->level_slice.pos[AXIS__Z] + z });
                    self->chunk_renderers[CHUNK_INDEX(x, y, z)] = chunk_renderer_new(self, chunk);
                }
            }
        }
    }
}