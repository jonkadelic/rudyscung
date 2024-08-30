#pragma once

#include "src/render/tessellator.h"
#include "src/render/level_renderer.h"
#include "src/world/chunk.h"

typedef struct chunk_renderer chunk_renderer_t;

chunk_renderer_t* const chunk_renderer_new(level_renderer_t* const level_renderer, chunk_t const* const chunk);

void chunk_renderer_delete(chunk_renderer_t* const self);

chunk_t const* const chunk_renderer_get_chunk(chunk_renderer_t const* const self);

bool chunk_renderer_is_ready(chunk_renderer_t const* const self);

void chunk_renderer_build(chunk_renderer_t* const self, tessellator_t* const tessellator);

void chunk_renderer_draw(chunk_renderer_t const* const self);
