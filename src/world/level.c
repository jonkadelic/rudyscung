#include "./level.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

#include "../util.h"
#include "chunk.h"
#include "entity/ecs.h"
#include "entity/ecs_components.h"
#include "entity/ecs_systems.h"
#include "side.h"
#include "tile.h"
#include "tile_shape.h"
#include "gen/level_gen.h"

#define CHUNK_INDEX(pos) (((pos[AXIS__Y]) * self->size[AXIS__Z] * self->size[AXIS__X]) + ((pos[AXIS__Z]) * self->size[AXIS__X]) + (pos[AXIS__X]))
#define TO_CHUNK_SPACE(coord) ((coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(coord) ((coord) * CHUNK_SIZE)
#define TO_CHUNK_SPACE_ARR(pos) ((size_chunks_t[NUM_AXES]) { pos[AXIS__X] / CHUNK_SIZE, pos[AXIS__Y] / CHUNK_SIZE, pos[AXIS__Z] / CHUNK_SIZE })
#define TO_TILE_SPACE_ARR(pos) ((size_t[NUM_AXES]) { pos[AXIS__X] * CHUNK_SIZE, pos[AXIS__Y] * CHUNK_SIZE, pos[AXIS__Z] * CHUNK_SIZE }) 
#define TO_POS_IN_CHUNK_ARR(pos) ((size_t[NUM_AXES]) { pos[AXIS__X] % CHUNK_SIZE, pos[AXIS__Y] % CHUNK_SIZE, pos[AXIS__Z] % CHUNK_SIZE }) 

struct level {
    size_chunks_t size[NUM_AXES];
    chunk_t** chunks;
    level_gen_t* level_gen;
    ecs_t* ecs;
    entity_t player;
    entity_t trees[NUM_TREES];
};

level_t* const level_new(size_chunks_t const size[NUM_AXES]) {
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(size[a] > 0);
    }

    level_t* const self = malloc(sizeof(level_t));
    assert(self != nullptr);

    memcpy(self->size, size, sizeof(size_chunks_t) * NUM_AXES);

    struct timeval time;
    gettimeofday(&time, nullptr);
    unsigned int seed = time.tv_sec * 1000 + time.tv_usec / 1000;
    self->level_gen = level_gen_new(seed);

    self->chunks = malloc(sizeof(chunk_t*) * size[AXIS__X] * size[AXIS__Y] * size[AXIS__Z]);
    assert(self->chunks != nullptr);

    for (size_chunks_t x = 0; x < size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < size[AXIS__Z]; z++) {
                size_chunks_t const i_pos[NUM_AXES] = { x, y, z };
                self->chunks[CHUNK_INDEX(i_pos)] = chunk_new(i_pos);
                level_gen_generate(self->level_gen, self->chunks[CHUNK_INDEX(i_pos)]);                
            }
        }
    }

    level_gen_smooth(self->level_gen, self);

    self->ecs = ecs_new();
    ecs_attach_system(self->ecs, ECS_COMPONENT__AABB, ecs_system_collision);
    ecs_attach_system(self->ecs, ECS_COMPONENT__VEL, ecs_system_velocity);
    ecs_attach_system(self->ecs, ECS_COMPONENT__VEL, ecs_system_friction);
    ecs_attach_system(self->ecs, ECS_COMPONENT__GRAVITY, ecs_system_gravity);

    self->player = ecs_new_entity(self->ecs);
    ecs_component_pos_t* const player_pos = ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__POS);
    ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__VEL);
    ecs_component_rot_t* const player_rot = ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__ROT);
    ecs_component_aabb_t* const player_aabb = ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__AABB);
    ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__GRAVITY);
    
    player_pos->pos[AXIS__X] = 24.0f;
    player_pos->pos[AXIS__Y] = 100.0f;
    player_pos->pos[AXIS__Z] = 24.0f;

    player_rot->rot[ROT_AXIS__Y] = M_PI / 4 * 3;

    float const player_aabb_min[NUM_AXES] = { -0.4f, -1.8f, -0.4f };
    float const player_aabb_max[NUM_AXES] = { 0.4f, 0.2f, 0.4f };
    aabb_set_bounds(player_aabb->aabb, player_aabb_min, player_aabb_max);

    srand(seed);
    for (size_t i = 0; i < NUM_TREES; i++) {
        size_t i_tree_pos[NUM_AXES] = {
            rand() / (RAND_MAX / (self->size[AXIS__X] * CHUNK_SIZE)),
            0,
            rand() / (RAND_MAX / (self->size[AXIS__Z] * CHUNK_SIZE))
        };
        for (size_t y = (self->size[AXIS__Y] * CHUNK_SIZE) - 1; y >= 0; y--) {
            i_tree_pos[AXIS__Y] = y;
            if (level_get_tile(self, i_tree_pos) != tile_get(TILE_ID__AIR)) {
                i_tree_pos[AXIS__Y] = y + 1;
                break;
            }
        }

        self->trees[i] = ecs_new_entity(self->ecs);
        ecs_component_pos_t* const tree_pos = ecs_attach_component(self->ecs, self->trees[i], ECS_COMPONENT__POS);
        ecs_component_sprite_t* const tree_sprite = ecs_attach_component(self->ecs, self->trees[i], ECS_COMPONENT__SPRITE);

        tree_pos->pos[AXIS__X] = i_tree_pos[AXIS__X] + 0.5f;
        tree_pos->pos[AXIS__Y] = i_tree_pos[AXIS__Y];
        tree_pos->pos[AXIS__Z] = i_tree_pos[AXIS__Z] + 0.5f;
        
        tree_sprite->sprite = SPRITE__TREE;
    }

    return self;
}

void level_delete(level_t* const self) {
    assert(self != nullptr);

    for (size_chunks_t x = 0; x < self->size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < self->size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < self->size[AXIS__Z]; z++) {
                size_chunks_t const i_pos[NUM_AXES] = { x, y, z };
                chunk_delete(self->chunks[CHUNK_INDEX(i_pos)]);
                self->chunks[CHUNK_INDEX(i_pos)] = nullptr;
            }
        }
    }
    free(self->chunks);

    level_gen_delete(self->level_gen);

    ecs_delete(self->ecs);

    free(self);
}

void level_get_size(level_t const* const self, size_chunks_t size[NUM_AXES]) {
    assert(self != nullptr);

    memcpy(size, self->size, sizeof(size_chunks_t) * NUM_AXES);
}

chunk_t* const level_get_chunk(level_t const* const self, size_chunks_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < self->size[a]);
    }

    return self->chunks[CHUNK_INDEX(pos)];
}

tile_t const* const level_get_tile(level_t const* const self, size_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(self->size[a]));
    }

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE_ARR(pos));

    return chunk_get_tile(chunk, TO_POS_IN_CHUNK_ARR(pos));
}

void level_set_tile(level_t* const self, size_t const pos[NUM_AXES], tile_t const* const tile) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(self->size[a]));
    }
    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE_ARR(pos));

    chunk_set_tile(chunk, TO_POS_IN_CHUNK_ARR(pos), tile);
}

tile_shape_t const level_get_tile_shape(level_t const* const self, size_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(self->size[a]));
    }

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE_ARR(pos));

    return chunk_get_tile_shape(chunk, TO_POS_IN_CHUNK_ARR(pos));
}

void level_set_tile_shape(level_t* const self, size_t const pos[NUM_AXES], tile_shape_t const shape) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(self->size[a]));
    }

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE_ARR(pos));

    chunk_set_tile_shape(chunk, TO_POS_IN_CHUNK_ARR(pos), shape);
}

void level_tick(level_t* const self) {
    assert(self != nullptr);

    ecs_tick(self->ecs, self);
}

ecs_t* const level_get_ecs(level_t* const self) {
    assert(self != nullptr);

    return self->ecs;
}

entity_t const level_get_player(level_t const* const self) {
    assert(self != nullptr);

    return self->player;
}

float const level_get_nearest_face_on_axis(level_t const* const self, float const pos[NUM_AXES], side_t const side, float const max_range) {
    assert(self != nullptr);
    assert(side >= 0 && side < NUM_SIDES);
    assert(max_range > 0);

    int o[NUM_AXES];
    side_get_offsets(side, o);

    size_chunks_t level_size[NUM_AXES];
    level_get_size(self, level_size);

    tile_t const* const air_tile = tile_get(TILE_ID__AIR);

    float i_pos[NUM_AXES];
    for (axis_t a = 0; a < NUM_AXES; a++) {
        i_pos[a] = (int) pos[a] + 0.5f;
    }

    for (axis_t a = 0; a < NUM_AXES; a++) {
        if (o[a] != 0) {
            float d = i_pos[a] - pos[a];

            while (true) {
                if (i_pos[a] < 0.0f || i_pos[a] >= level_size[a] * CHUNK_SIZE) {
                    if (o[a] < 0) {
                        return ceil(i_pos[a]);
                    } else {
                        return floor(i_pos[a]);
                    }
                }

                for (axis_t aa = 0; aa < NUM_AXES; aa++) {
                    if (i_pos[aa] < 0.0f || i_pos[aa] >= level_size[aa] * CHUNK_SIZE) {
                        return NAN;
                    }
                }
                tile_t const* const tile = level_get_tile(self, VEC_CAST(size_t, i_pos));
                if (tile != air_tile) {
                    break;
                }

                if ((o[a] < 0 && d < -max_range) || (o[a] > 0 && d > max_range)) {
                    return NAN;
                }

                if (max_range < 0.5f) {
                    d += o[a] * max_range;
                    i_pos[a] += o[a] * max_range;
                } else {
                    d += o[a];
                    i_pos[a] += o[a];
                }
            }

            float final_pos[NUM_AXES];
            for (axis_t aa = 0; aa < NUM_AXES; aa++) {
                if (a == aa) {
                    if (o[aa] < 0) {
                        final_pos[aa] = ceil(i_pos[aa]);
                    } else {
                        final_pos[aa] = floor(i_pos[aa]);
                    }
                } else {
                    final_pos[aa] = pos[aa];
                }
            }

            // tile_shape_t const tile_shape = level_get_tile_shape(self, VEC_CAST(size_t, i_pos));

            // float pos2[2];
            // side_map_point(side, final_pos, pos2);
            // pos2[0] = map_to_0_1(pos2[0]);
            // pos2[1] = map_to_0_1(pos2[1]);
            
            // float inner = tile_shape_get_inner_distance(tile_shape, side, pos2);
            // if (inner == INFINITY) {
            //     return NAN;
            // } else {
            //     inner = absf(inner);
            //     if (o[a] < 0) {
            //         final_pos[a] -= inner;
            //     } else {
            //         final_pos[a] += inner;
            //     }
            // }

            return final_pos[a];
        }
    }

    return NAN;
}

entity_t const level_get_tree(level_t const* const self, size_t const index) {
    assert(self != nullptr);
    assert(index >= 0 && index < NUM_TREES);

    return self->trees[index];
}