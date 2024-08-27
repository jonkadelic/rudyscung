#include "./raycast.h"

#include <assert.h>

void raycast_cast_in_level(raycast_t* const self, level_t const* const level, float const pos[NUM_ROT_AXES], float const rot[NUM_ROT_AXES]) {
    assert(self != nullptr);
    assert(level != nullptr);

    float raypos[NUM_AXES] = { pos[AXIS__X], pos[AXIS__Y], pos[AXIS__Z] };

    float dir[NUM_AXES] = { -cosf(rot[ROT_AXIS__X]) * -sinf(rot[ROT_AXIS__Y]), -sinf(rot[ROT_AXIS__X]), -cosf(rot[ROT_AXIS__X]) * cosf(rot[ROT_AXIS__Y]) };

    float length = sqrtf(dir[AXIS__X] * dir[AXIS__X] + dir[AXIS__Y] * dir[AXIS__Y] + dir[AXIS__Z] * dir[AXIS__Z]);

    if (length != 0) {
        dir[AXIS__X] /= length;
        dir[AXIS__Y] /= length;
        dir[AXIS__Z] /= length;
    }

    float raydir[NUM_AXES] = { dir[AXIS__X], dir[AXIS__Y], dir[AXIS__Z] };
    
    float const ray_scale = 0.1f;
    float scaled_ray[NUM_AXES] = { raydir[AXIS__X] * ray_scale, raydir[AXIS__Y] * ray_scale, raydir[AXIS__Z] * ray_scale };

    float const max_range = 10.0f;

    size_t level_size[NUM_AXES];
    level_get_size(level, level_size);
    size_t level_size_tiles[NUM_AXES] = { level_size[AXIS__X] * CHUNK_SIZE, level_size[AXIS__Y] * CHUNK_SIZE, level_size[AXIS__Z] * CHUNK_SIZE };

    self->hit = false;

    for (float range = 0.0f; range < max_range; range += ray_scale) {
        size_t tile_pos[NUM_AXES] = { (size_t) floorf(raypos[AXIS__X]), (size_t) floorf(raypos[AXIS__Y]), (size_t) floorf(raypos[AXIS__Z]) };

        if (tile_pos[AXIS__X] < 0 || tile_pos[AXIS__Y] < 0 || tile_pos[AXIS__Z] < 0) {
            break;
        }

        if (tile_pos[AXIS__X] >= level_size_tiles[AXIS__X] || tile_pos[AXIS__Y] >= level_size_tiles[AXIS__Y] || tile_pos[AXIS__Z] >= level_size_tiles[AXIS__Z]) {
            break;
        }

        tile_t const tile = level_get_tile(level, tile_pos);

        if (tile != TILE__AIR) {
            self->hit = true;
            self->tile = tile;
            self->side = SIDE__TOP;

            self->pos[AXIS__X] = raypos[AXIS__X];
            self->pos[AXIS__Y] = raypos[AXIS__Y];
            self->pos[AXIS__Z] = raypos[AXIS__Z];

            self->tile_pos[AXIS__X] = tile_pos[AXIS__X];
            self->tile_pos[AXIS__Y] = tile_pos[AXIS__Y];
            self->tile_pos[AXIS__Z] = tile_pos[AXIS__Z];

            break;
        }

        raypos[AXIS__X] += scaled_ray[AXIS__X];
        raypos[AXIS__Y] += scaled_ray[AXIS__Y];
        raypos[AXIS__Z] += scaled_ray[AXIS__Z];
    }
}