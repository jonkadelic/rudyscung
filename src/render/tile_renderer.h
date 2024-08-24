#pragma once

#include "../world/tile.h"
#include "../world/tile_shape.h"
#include "./tessellator.h"

void tile_renderer_render_tile(tessellator_t* const self, int const x, int const y, int const z, tile_t const* const tile, tile_shape_t const tile_shape, bool is_side_occluded[NUM_SIDES]);

void tile_renderer_render_tile_f(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, tile_shape_t const tile_shape, bool is_side_occluded[NUM_SIDES]);