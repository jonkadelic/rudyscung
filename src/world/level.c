#include "./level.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include "src/util/util.h"
#include "src/world/chunk.h"
#include "src/world/entity/ecs.h"
#include "src/world/entity/ecs_components.h"
#include "src/world/entity/ecs_systems.h"
#include "src/world/side.h"
#include "src/world/tile.h"
#include "src/world/tile_shape.h"
#include "src/world/tile.h"
#include "src/world/tile_shape.h"
#include "src/world/gen/level_gen.h"
#include "src/util/random.h"
#include "src/util/logger.h"

#define CHUNK_INDEX(pos) (((pos[AXIS__Y]) * self->size[AXIS__Z] * self->size[AXIS__X]) + ((pos[AXIS__Z]) * self->size[AXIS__X]) + (pos[AXIS__X]))
#define TO_CHUNK_SPACE(coord) ((coord) / CHUNK_SIZE)
#define TO_TILE_SPACE(coord) ((coord) * CHUNK_SIZE)
#define TO_CHUNK_SPACE_ARR(pos) ((size_chunks_t[NUM_AXES]) { pos[AXIS__X] / CHUNK_SIZE, pos[AXIS__Y] / CHUNK_SIZE, pos[AXIS__Z] / CHUNK_SIZE })
#define TO_TILE_SPACE_ARR(pos) ((size_t[NUM_AXES]) { pos[AXIS__X] * CHUNK_SIZE, pos[AXIS__Y] * CHUNK_SIZE, pos[AXIS__Z] * CHUNK_SIZE }) 
#define TO_POS_IN_CHUNK_ARR(pos) ((size_t[NUM_AXES]) { pos[AXIS__X] % CHUNK_SIZE, pos[AXIS__Y] % CHUNK_SIZE, pos[AXIS__Z] % CHUNK_SIZE }) 

struct level {
    size_chunks_t size[NUM_AXES];
    chunk_t** chunks;
    bool* is_chunk_dirty;
    level_gen_t* level_gen;
    ecs_t* ecs;
    uint64_t seed;
    random_t* rand;
    entity_t player;
};

level_t* const level_new(size_chunks_t const size[NUM_AXES]) {
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(size[a] > 0);
    }

    LOG_DEBUG("level_t: creating new level [%zu x %zu x %zu].", size[AXIS__X], size[AXIS__Y], size[AXIS__Z]);

    level_t* const self = malloc(sizeof(level_t));
    assert(self != nullptr);

    memcpy(self->size, size, sizeof(size_chunks_t) * NUM_AXES);

    self->seed = (uint64_t) get_time_ms();
    self->level_gen = level_gen_new(self->seed);

    uint64_t const start_time = get_time_ms();

    self->chunks = malloc(sizeof(chunk_t*) * size[AXIS__X] * size[AXIS__Y] * size[AXIS__Z]);
    assert(self->chunks != nullptr);

    LOG_DEBUG("level_t: allocated %zu chunks.", size[AXIS__X] * size[AXIS__Y] * size[AXIS__Z]);

    for (size_chunks_t x = 0; x < size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < size[AXIS__Z]; z++) {
                size_chunks_t const i_pos[NUM_AXES] = { x, y, z };
                self->chunks[CHUNK_INDEX(i_pos)] = chunk_new(i_pos);
                level_gen_generate(self->level_gen, self->chunks[CHUNK_INDEX(i_pos)]);                
            }
        }
    }

    self->is_chunk_dirty = malloc(sizeof(bool) * size[AXIS__X] * size[AXIS__Y] * size[AXIS__Z]);
    assert(self->is_chunk_dirty != nullptr);
    for (size_chunks_t x = 0; x < size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < size[AXIS__Z]; z++) {
                size_chunks_t const i_pos[NUM_AXES] = { x, y, z };
                self->is_chunk_dirty[CHUNK_INDEX(i_pos)] = false;
            }
        }
    }

    level_gen_smooth(self->level_gen, self);

    self->ecs = ecs_new();
    ecs_attach_system(self->ecs, ECS_COMPONENT__VEL, ecs_system_velocity);
    ecs_attach_system(self->ecs, ECS_COMPONENT__VEL, ecs_system_friction);
    ecs_attach_system(self->ecs, ECS_COMPONENT__GRAVITY, ecs_system_gravity);
    ecs_attach_system(self->ecs, ECS_COMPONENT__MOVE_RANDOM, ecs_system_move_random);

    self->player = ecs_new_entity(self->ecs);
    ecs_component_pos_t* const player_pos = ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__POS);
    ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__VEL);
    ecs_component_rot_t* const player_rot = ecs_attach_component(self->ecs, self->player, ECS_COMPONENT__ROT);
    
    player_pos->pos[AXIS__X] = (size[AXIS__X] / 2.0f) * CHUNK_SIZE;
    player_pos->pos[AXIS__Y] = 120.0f;
    player_pos->pos[AXIS__Z] = (size[AXIS__Z] / 2.0f) * CHUNK_SIZE;

    player_rot->rot[ROT_AXIS__Y] = M_PI / 4 * 3;
    player_rot->rot[ROT_AXIS__X] = M_PI / 4 * 2;

    self->rand = random_new(self->seed);
    for (size_t i = 0; i < NUM_TREES; i++) {
        size_t i_tree_pos[NUM_AXES] = {
            random_next_int_bounded(self->rand, self->size[AXIS__X] * CHUNK_SIZE - 1),
            0,
            random_next_int_bounded(self->rand, self->size[AXIS__Z] * CHUNK_SIZE - 1)
        };
        for (size_t y = (self->size[AXIS__Y] * CHUNK_SIZE) - 1; y >= 0; y--) {
            i_tree_pos[AXIS__Y] = y;
            if (level_get_tile(self, i_tree_pos) != TILE__AIR) {
                i_tree_pos[AXIS__Y] = y + 1;
                break;
            }
        }

        size_t const below_pos[NUM_AXES] = { i_tree_pos[AXIS__X], i_tree_pos[AXIS__Y] - 1, i_tree_pos[AXIS__Z] };

        if (level_get_tile(self, below_pos) != TILE__GRASS) {
            i--;
            continue;
        }
        float y_offset = 0.0f;
        tile_shape_t const below_tile_shape = level_get_tile_shape(self, below_pos);
        if (below_tile_shape == TILE_SHAPE__RAMP_NORTH || below_tile_shape == TILE_SHAPE__RAMP_SOUTH || below_tile_shape == TILE_SHAPE__RAMP_WEST || below_tile_shape == TILE_SHAPE__RAMP_EAST) {
            y_offset = -0.5f;
        }
        if (below_tile_shape == TILE_SHAPE__CORNER_A_NORTH_WEST || below_tile_shape == TILE_SHAPE__CORNER_A_SOUTH_WEST || below_tile_shape == TILE_SHAPE__CORNER_A_NORTH_EAST || below_tile_shape == TILE_SHAPE__CORNER_A_SOUTH_EAST) {
            y_offset = -1.0f;
        }

        entity_t const tree = ecs_new_entity(self->ecs);
        ecs_component_pos_t* const tree_pos = ecs_attach_component(self->ecs, tree, ECS_COMPONENT__POS);
        ecs_component_sprite_t* const tree_sprite = ecs_attach_component(self->ecs, tree, ECS_COMPONENT__SPRITE);

        tree_pos->pos[AXIS__X] = i_tree_pos[AXIS__X] + 0.5f;
        tree_pos->pos[AXIS__Y] = i_tree_pos[AXIS__Y] + y_offset;
        tree_pos->pos[AXIS__Z] = i_tree_pos[AXIS__Z] + 0.5f;
        
        tree_sprite->sprite = SPRITE__TREE;
        tree_sprite->scale = 0.05f;
    }

    for (size_t i = 0; i < 100; i++) {
        size_t i_mob_pos[NUM_AXES] = {
            random_next_int_bounded(self->rand, self->size[AXIS__X] * CHUNK_SIZE - 1),
            120,
            random_next_int_bounded(self->rand, self->size[AXIS__Z] * CHUNK_SIZE - 1)
        };

        entity_t const mob = ecs_new_entity(self->ecs);
        ecs_component_pos_t* const mob_pos = ecs_attach_component(self->ecs, mob, ECS_COMPONENT__POS);
        ecs_component_rot_t* const mob_rot = ecs_attach_component(self->ecs, mob, ECS_COMPONENT__ROT);
        ecs_component_aabb_t* const mob_aabb = ecs_attach_component(self->ecs, mob, ECS_COMPONENT__AABB);
        ecs_attach_component(self->ecs, mob, ECS_COMPONENT__GRAVITY);
        ecs_attach_component(self->ecs, mob, ECS_COMPONENT__VEL);
        ecs_component_sprite_t* const mob_sprite = ecs_attach_component(self->ecs, mob, ECS_COMPONENT__SPRITE);
        ecs_attach_component(self->ecs, mob, ECS_COMPONENT__MOVE_RANDOM);

        mob_pos->pos[AXIS__X] = i_mob_pos[AXIS__X] + 0.5f;
        mob_pos->pos[AXIS__Y] = i_mob_pos[AXIS__Y];
        mob_pos->pos[AXIS__Z] = i_mob_pos[AXIS__Z] + 0.5f;

        mob_rot->rot[ROT_AXIS__Y] = M_PI * 2 * random_next_float(self->rand);

        aabb_set_bounds(mob_aabb->aabb, (float[NUM_AXES]) { -0.4f, 0.0f, -0.4f }, (float[NUM_AXES]) { 0.4f, 1.8f, 0.4f });

        mob_sprite->sprite = SPRITE__MOB;
        mob_sprite->scale = 0.075f;
    }

    uint64_t const end_time = get_time_ms();
    LOG_DEBUG("level_t: generated level in %lums.", end_time - start_time);

    LOG_DEBUG("level_t: initialized.");

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

    LOG_DEBUG("level_t: deleted.");
}

uint64_t const level_get_seed(level_t const* const self) {
    assert(self != nullptr);

    return self->seed;
}

void level_get_size(level_t const* const self, size_chunks_t size[NUM_AXES]) {
    assert(self != nullptr);

    memcpy(size, self->size, sizeof(size_chunks_t) * NUM_AXES);
}

bool const level_is_tile_oob(level_t const* const self, size_t const pos[NUM_AXES]) {
    assert(self != nullptr);

    return
        (pos[AXIS__X] >= self->size[AXIS__X] * CHUNK_SIZE) ||
        (pos[AXIS__Y] >= self->size[AXIS__Y] * CHUNK_SIZE) ||
        (pos[AXIS__Z] >= self->size[AXIS__Z] * CHUNK_SIZE);
}

random_t* const level_get_random(level_t* const self) {
    assert(self != nullptr);

    return self->rand;
}

bool const level_is_chunk_dirty(level_t const* const self, size_chunks_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < self->size[a]);
    }

    return self->is_chunk_dirty[CHUNK_INDEX(pos)];
}

chunk_t* const level_get_chunk(level_t const* const self, size_chunks_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < self->size[a]);
    }

    return self->chunks[CHUNK_INDEX(pos)];
}

tile_t const level_get_tile(level_t const* const self, size_t const pos[NUM_AXES]) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(self->size[a]));
    }

    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE_ARR(pos));

    return chunk_get_tile(chunk, TO_POS_IN_CHUNK_ARR(pos));
}

void level_set_tile(level_t* const self, size_t const pos[NUM_AXES], tile_t const tile) {
    assert(self != nullptr);
    for (axis_t a = 0; a < NUM_AXES; a++) {
        assert(pos[a] >= 0 && pos[a] < TO_TILE_SPACE(self->size[a]));
    }
    chunk_t* const chunk = level_get_chunk(self, TO_CHUNK_SPACE_ARR(pos));

    chunk_set_tile(chunk, TO_POS_IN_CHUNK_ARR(pos), tile);

    self->is_chunk_dirty[CHUNK_INDEX(TO_CHUNK_SPACE_ARR(pos))] = true;
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

    self->is_chunk_dirty[CHUNK_INDEX(TO_CHUNK_SPACE_ARR(pos))] = true;
}

void level_tick(level_t* const self) {
    assert(self != nullptr);

    ecs_tick(self->ecs, self);

    for (size_chunks_t x = 0; x < self->size[AXIS__X]; x++) {
        for (size_chunks_t y = 0; y < self->size[AXIS__Y]; y++) {
            for (size_chunks_t z = 0; z < self->size[AXIS__Z]; z++) {
                size_chunks_t const i_pos[NUM_AXES] = { x, y, z };
                self->is_chunk_dirty[CHUNK_INDEX(i_pos)] = false;
            }
        }
    }
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
                tile_t const tile = level_get_tile(self, VEC_CAST(size_t, i_pos));
                if (tile != TILE__AIR) {
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
