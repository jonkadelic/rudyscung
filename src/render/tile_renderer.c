#include "./tile_renderer.h"

#include <assert.h>
#include <math.h>

#include "src/render/tessellator.h"
#include "src/render/tile_texture_dispatcher.h"
#include "src/util/util.h"

#define TERRAIN_SIZE_PIXELS 256
#define TILE_SIZE_PIXELS 16
#define TO_PIXEL_SPACE(tile_coord) ((tile_coord) * TILE_SIZE_PIXELS)
#define TO_TEXTURE_SPACE(pixel_coord) ((pixel_coord) / (float) TERRAIN_SIZE_PIXELS)

typedef void (*tile_shape_renderer_t)(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);

static void render_shape_flat(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_north(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_south(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_ramp_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_north_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_south_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_north_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_a_south_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_north_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_south_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_north_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);
static void render_shape_corner_b_south_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]);

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

typedef void (*tile_side_renderer_t)(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);

static void render_side_north(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);
static void render_side_south(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);
static void render_side_bottom(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);
static void render_side_top(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);
static void render_side_west(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);
static void render_side_east(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]);

static tile_side_renderer_t const SIDE_RENDERERS[NUM_SIDES] = {
    [SIDE__NORTH] = render_side_north,
    [SIDE__SOUTH] = render_side_south,
    [SIDE__BOTTOM] = render_side_bottom,
    [SIDE__TOP] = render_side_top,
    [SIDE__WEST] = render_side_west,
    [SIDE__EAST] = render_side_east
};

void tile_renderer_render_tile(tessellator_t* const self, int const pos[NUM_AXES], tile_t const tile, tile_shape_t const tile_shape, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    tile_renderer_render_tile_f(self, (float[NUM_AXES]) { floor(pos[AXIS__X]), floor(pos[AXIS__Y]), floor(pos[AXIS__Z]) }, tile, tile_shape, color, is_side_occluded);
}

void tile_renderer_render_tile_f(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, tile_shape_t const tile_shape, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    assert(self != nullptr);
    assert(tile >= 0 && tile < NUM_TILES);
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
        SHAPE_RENDERERS[tile_shape](self, pos, tile, color, is_side_occluded);
    }
}

#define X_MIN (pos[AXIS__X] + 0.0f)
#define X_MID (pos[AXIS__X] + 0.5f)
#define X_MAX (pos[AXIS__X] + 1.0f)
#define Y_MIN (pos[AXIS__Y] + 0.0f)
#define Y_MID (pos[AXIS__Y] + 0.5f)
#define Y_MAX (pos[AXIS__Y] + 1.0f)
#define Z_MIN (pos[AXIS__Z] + 0.0f)
#define Z_MID (pos[AXIS__Z] + 0.5f)
#define Z_MAX (pos[AXIS__Z] + 1.0f)

#define INIT_UV(x, y) \
    size_t px_min = TO_PIXEL_SPACE(x) + 0; \
    size_t px_mid = TO_PIXEL_SPACE(x) + (TILE_SIZE_PIXELS / 2); \
    size_t px_max = TO_PIXEL_SPACE(x) + TILE_SIZE_PIXELS; \
    size_t py_min = TO_PIXEL_SPACE(y) + 0; \
    size_t py_mid = TO_PIXEL_SPACE(y) + (TILE_SIZE_PIXELS / 2); \
    size_t py_max = TO_PIXEL_SPACE(y) + TILE_SIZE_PIXELS; \
    float u_min = TO_TEXTURE_SPACE(px_min); \
    float u_mid = TO_TEXTURE_SPACE(px_mid); \
    float u_max = TO_TEXTURE_SPACE(px_max); \
    float v_min = TO_TEXTURE_SPACE(py_min); \
    float v_mid = TO_TEXTURE_SPACE(py_mid); \
    float v_max = TO_TEXTURE_SPACE(py_max);

static float const BRIGHTNESSES[NUM_SIDES][NUM_AXES] = {
    [SIDE__NORTH] = { 0.8f, 0.8f, 0.8f },
    [SIDE__SOUTH] = { 0.8f, 0.8f, 0.8f },
    [SIDE__BOTTOM] = { 0.5f, 0.5f, 0.5f },
    [SIDE__TOP] = { 1.0f, 1.0f, 1.0f },
    [SIDE__WEST] = { 0.6f, 0.6f, 0.6f },
    [SIDE__EAST] = { 0.6f, 0.6f, 0.6f }
};

static void render_shape_flat(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);

        SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(BRIGHTNESSES[i], color));
    }
}

static void render_shape_ramp_north(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__SOUTH][j] + ((BRIGHTNESSES[SIDE__TOP][j] - BRIGHTNESSES[SIDE__SOUTH][j]) / 2);
            }

            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
        } else if (i == SIDE__WEST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__EAST) {
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__SOUTH) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_ramp_south(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__NORTH][j] + ((BRIGHTNESSES[SIDE__TOP][j] - BRIGHTNESSES[SIDE__NORTH][j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__WEST) {
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__EAST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
        } else if (i == SIDE__NORTH) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_ramp_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__EAST][j] + ((BRIGHTNESSES[SIDE__TOP][j] - BRIGHTNESSES[SIDE__EAST][j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
        } else if (i == SIDE__NORTH) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__SOUTH) {
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__EAST) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_ramp_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__WEST][j] + ((BRIGHTNESSES[SIDE__TOP][j] - BRIGHTNESSES[SIDE__WEST][j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__NORTH) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__SOUTH) {
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__WEST) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_a_north_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__SOUTH][j] + BRIGHTNESSES[SIDE__EAST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
        } else if (i == SIDE__NORTH) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__WEST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__SOUTH || i == SIDE__EAST || i == SIDE__BOTTOM) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_a_south_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__NORTH][j] + BRIGHTNESSES[SIDE__EAST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
        } else if (i == SIDE__SOUTH) {
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__WEST) {
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__NORTH || i == SIDE__EAST || i == SIDE__BOTTOM) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_a_north_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__SOUTH][j] + BRIGHTNESSES[SIDE__WEST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__NORTH) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__EAST) {
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__SOUTH || i == SIDE__WEST || i == SIDE__BOTTOM) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_a_south_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__NORTH][j] + BRIGHTNESSES[SIDE__WEST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__SOUTH) {
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__EAST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
        } else if (i == SIDE__NORTH || i == SIDE__WEST || i == SIDE__BOTTOM) {
            // Do nothing
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_b_north_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__SOUTH][j] + BRIGHTNESSES[SIDE__EAST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);

            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__TOP][j];
            }

            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
        } else if (i == SIDE__SOUTH) {
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__EAST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_b_south_west(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__NORTH][j] + BRIGHTNESSES[SIDE__WEST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);

            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__TOP][j];
            }

            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
        } else if (i == SIDE__NORTH) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__EAST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_b_north_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__SOUTH][j] + BRIGHTNESSES[SIDE__WEST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);

            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__TOP][j];
            }

            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else if (i == SIDE__SOUTH) {
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__WEST) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_shape_corner_b_south_east(tessellator_t* const self, float const pos[NUM_AXES], tile_t const tile, float const color[3], bool is_side_occluded[NUM_SIDES]) {
    for (size_t i = 0; i < NUM_SIDES; i++) {
        if (is_side_occluded[i]) {
            continue;
        }

        tile_texture_coords_t tex;
        tile_texture_dispatcher_get_tile_texture_coords(tile, i, &tex);
        INIT_UV(tex.x, tex.y);

        float br[3] = { BRIGHTNESSES[i][0], BRIGHTNESSES[i][1], BRIGHTNESSES[i][2] };

        if (i == SIDE__TOP) {
            for (size_t j = 0; j < 3; j++) {
                br[j] = (BRIGHTNESSES[SIDE__NORTH][j] + BRIGHTNESSES[SIDE__WEST][j]) / 2;
                br[j] += ((BRIGHTNESSES[SIDE__TOP][j] - br[j]) / 2);
            }

            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);

            for (size_t j = 0; j < 3; j++) {
                br[j] = BRIGHTNESSES[SIDE__TOP][j];
            }

            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);

        } else if (i == SIDE__NORTH) {
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_min);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
        } else if (i == SIDE__WEST) {
            tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_min);
            tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_min, v_max);
            tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, br[0] * color[0], br[1] * color[1], br[2] * color[2], u_max, v_max);
        } else {
            SIDE_RENDERERS[i](self, pos, &tex, VEC_MUL(br, color));
        }
    }
}

static void render_side_north(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]) {
    INIT_UV(tex->x, tex->y);

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, color[0], color[1], color[2], u_max, v_min);
}

static void render_side_south(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]) {
    INIT_UV(tex->x, tex->y);

    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, color[0], color[1], color[2], u_max, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, color[0], color[1], color[2], u_max, v_min);
}

static void render_side_bottom(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]) {
    INIT_UV(tex->x, tex->y);

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, color[0], color[1], color[2], u_max, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, color[0], color[1], color[2], u_max, v_max);
}

static void render_side_top(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]) {
    INIT_UV(tex->x, tex->y);

    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, color[0], color[1], color[2], u_max, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, color[0], color[1], color[2], u_min, v_min);
}

static void render_side_west(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]) {
    INIT_UV(tex->x, tex->y);

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MIN, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, color[0], color[1], color[2], u_max, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MIN, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MIN, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MIN, color[0], color[1], color[2], u_max, v_min);
}

static void render_side_east(tessellator_t* const self, float const pos[NUM_AXES], tile_texture_coords_t const* const tex, float const color[3]) {
    INIT_UV(tex->x, tex->y);

    tessellator_buffer_vct(self, X_MIN, Y_MIN, Z_MAX, color[0], color[1], color[2], u_min, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, color[0], color[1], color[2], u_min, v_min);
    tessellator_buffer_vct(self, X_MAX, Y_MIN, Z_MAX, color[0], color[1], color[2], u_max, v_max);
    tessellator_buffer_vct(self, X_MAX, Y_MAX, Z_MAX, color[0], color[1], color[2], u_max, v_min);
    tessellator_buffer_vct(self, X_MIN, Y_MAX, Z_MAX, color[0], color[1], color[2], u_min, v_min);
}
