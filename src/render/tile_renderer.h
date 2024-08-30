#pragma once

#include "src/world/tile.h"
#include "src/world/tile_shape.h"
#include "src/render/tessellator.h"

void tile_renderer_render_tile(tessellator_t* const tessellator, int const pos[NUM_AXES], tile_t const tile, tile_shape_t const tile_shape, float const color[3], bool is_side_occluded[NUM_SIDES]);

void tile_renderer_render_tile_f(tessellator_t* const tessellator, float const pos[NUM_AXES], tile_t const tile, tile_shape_t const tile_shape, float const color[3], bool is_side_occluded[NUM_SIDES]);