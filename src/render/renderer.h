#pragma once

#include "src/render/camera.h"
#include "src/render/level_renderer.h"

typedef struct client client_t;

typedef struct renderer renderer_t;

renderer_t* const renderer_new(client_t* const client);

void renderer_delete(renderer_t* const self);

void renderer_level_changed(renderer_t* const self);

level_renderer_t* const renderer_get_level_renderer(renderer_t* const self);

void renderer_tick(renderer_t* const self);

void renderer_render(renderer_t* const self, camera_t* const camera, float const partial_tick);
