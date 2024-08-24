#include "./tile_renderer.h"

#include <assert.h>
#include <math.h>

#include "./tessellator.h"
#include "./tile_texture_dispatcher.h"

#define TERRAIN_SIZE_PIXELS 256
#define TILE_SIZE_PIXELS 16
#define TO_PIXEL_SPACE(tile_coord) ((tile_coord) * TILE_SIZE_PIXELS)
#define TO_TEXTURE_SPACE(pixel_coord) ((pixel_coord) / (float) TERRAIN_SIZE_PIXELS)

typedef void (*tile_shape_renderer_t)(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);

static void render_shape_flat(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_north(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_south(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_north_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_south_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_north_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_south_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_north_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_south_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_north_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_south_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]);

static tile_shape_renderer_t const SHAPE_RENDERERS[NUM_TILE_SHAPES] = {
    [TILE_SHAPE__NO_RENDER] = nullptr,
    [TILE_SHAPE__FLAT] = render_shape_flat,
    [TILE_SHAPE__RAMP_NORTH] = render_shape_ramp_north,
    [TILE_SHAPE__RAMP_SOUTH] = render_shape_ramp_south,
    [TILE_SHAPE__RAMP_WEST] = render_shape_ramp_west,
    [TILE_SHAPE__RAMP_EAST] = render_shape_ramp_east,
    [TILE_SHAPE__CORNER_A_NORTH_WEST] = render_shape_corner_a_north_west,
    [TILE_SHAPE__CORNER_A_SOUTH_WEST] = render_shape_corner_a_south_west,
    [TILE_SHAPE__CORNER_A_NORTH_EAST] = render_shape_corner_a_north_east,
    [TILE_SHAPE__CORNER_A_SOUTH_EAST] = render_shape_corner_a_south_east,
    [TILE_SHAPE__CORNER_B_NORTH_WEST] = render_shape_corner_b_north_west,
    [TILE_SHAPE__CORNER_B_SOUTH_WEST] = render_shape_corner_b_south_west,
    [TILE_SHAPE__CORNER_B_NORTH_EAST] = render_shape_corner_b_north_east,
    [TILE_SHAPE__CORNER_B_SOUTH_EAST] = render_shape_corner_b_south_east,
};

typedef void (*tile_side_renderer_t)(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);

static void render_side_north(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);
static void render_side_south(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);
static void render_side_bottom(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);
static void render_side_top(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);
static void render_side_west(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);
static void render_side_east(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b);

static tile_side_renderer_t const SIDE_RENDERERS[NUM_SIDES] = {
    [SIDE__NORTH] = render_side_north,
    [SIDE__SOUTH] = render_side_south,
    [SIDE__BOTTOM] = render_side_bottom,
    [SIDE__TOP] = render_side_top,
    [SIDE__WEST] = render_side_west,
    [SIDE__EAST] = render_side_east
};

void tile_renderer_render_tile(tessellator_t* const self, int const x, int const y, int const z, tile_t const* const tile, tile_shape_t const tile_shape, bool is_side_occluded[NUM_SIDES]) {
    tile_renderer_render_tile_f(self, floor(x), floor(y), floor(z), tile, tile_shape, is_side_occluded);
}

void tile_renderer_render_tile_f(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, tile_shape_t const tile_shape, bool is_side_occluded[NUM_SIDES]) {
    assert(self != nullptr);
    assert(tile != nullptr);
    assert(tile_shape >= 0 && tile_shape < NUM_TILE_SHAPES);

    bool all_occluded = true;
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (!is_side_occluded[i]) {
            all_occluded = false;
            break;
        }
    }

    if (all_occluded) {
        return;
    }

    if (SHAPE_RENDERERS[tile_shape] != nullptr) {
        SHAPE_RENDERERS[tile_shape](self, x, y, z, tile, is_side_occluded);
    }
}

#define X_MIN (x + 0.0f)
#define X_MAX (x + 1.0f)
#define Y_MIN (y + 0.0f)
#define Y_MAX (y + 1.0f)
#define Z_MIN (z + 0.0f)
#define Z_MAX (z + 1.0f)

static void render_shape_flat(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {
    tile_texture_coords_t tex;
    tile_texture_dispatcher_get_tile_texture_coords(tile, &tex);

    float brightnesses[NUM_SIDES] = {
        [SIDE__NORTH] = 0.8f,
        [SIDE__SOUTH] = 0.8f,
        [SIDE__BOTTOM] = 0.5f,
        [SIDE__TOP] = 1.0f,
        [SIDE__WEST] = 0.6f,
        [SIDE__EAST] = 0.6f
    };

    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        SIDE_RENDERERS[i](self, x, y, z, &tex, brightnesses[i], brightnesses[i], brightnesses[i]);
    }
}

static void render_shape_ramp_north(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_ramp_south(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_ramp_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_ramp_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_a_north_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_a_south_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_a_north_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_a_south_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_b_north_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_b_south_west(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_b_north_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

static void render_shape_corner_b_south_east(tessellator_t* const self, float const x, float const y, float const z, tile_t const* const tile, bool is_side_occluded[NUM_SIDES]) {

}

#define INIT_UV() \
    size_t px_min = TO_PIXEL_SPACE(tex->x) + 0; \
    size_t px_max = TO_PIXEL_SPACE(tex->x) + TILE_SIZE_PIXELS; \
    size_t py_min = TO_PIXEL_SPACE(tex->y) + 0; \
    size_t py_max = TO_PIXEL_SPACE(tex->y) + TILE_SIZE_PIXELS; \
    float u_min = TO_TEXTURE_SPACE(px_min); \
    float u_max = TO_TEXTURE_SPACE(px_max); \
    float v_min = TO_TEXTURE_SPACE(py_min); \
    float v_max = TO_TEXTURE_SPACE(py_max);

static void render_side_north(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b) {
    INIT_UV();

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, r, g, b, u_max, v_min);
}

static void render_side_south(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b) {
    INIT_UV();

    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, r, g, b, u_max, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, r, g, b, u_max, v_min);
}

static void render_side_bottom(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b) {
    INIT_UV();

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, r, g, b, u_max, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, r, g, b, u_max, v_max);
}

static void render_side_top(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b) {
    INIT_UV();

    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, r, g, b, u_min, v_min);

    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, r, g, b, u_max, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, r, g, b, u_min, v_min);
}

static void render_side_west(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b) {
    INIT_UV();

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, r, g, b, u_max, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, r, g, b, u_max, v_min);
}

static void render_side_east(tessellator_t* const self, float const x, float const y, float const z, tile_texture_coords_t const* const tex, float const r, float const g, float const b) {
    INIT_UV();

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, r, g, b, u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, r, g, b, u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, r, g, b, u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, r, g, b, u_max, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, r, g, b, u_min, v_min);
}
