#pragma once

#include <stddef.h>

#include "src/world/side.h"

typedef struct client client_t;
typedef struct camera camera_t;

typedef struct sprites sprites_t;

typedef enum sprite {
    SPRITE__TREE,
    SPRITE__MOB,
    NUM_SPRITES
} sprite_t;

sprites_t* const sprites_new(client_t* const client);

void sprites_delete(sprites_t* const self);

void sprites_get_size(sprites_t const* const self, sprite_t const sprite, size_t size[2]);

void sprites_get_origin(sprites_t const* const self, sprite_t const sprite, float origin[2]);

void sprites_render(sprites_t const* const self, sprite_t const sprite, camera_t* const camera, float const scale, float const pos[NUM_AXES], float const rotation_offset, bool const rotate[NUM_ROT_AXES]);