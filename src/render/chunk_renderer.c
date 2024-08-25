#include "./chunk_renderer.h"

#include <assert.h>
#include <stdlib.h>

#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <SDL_timer.h>

#include "../world/chunk.h"
#include "level_renderer.h"
#include "tessellator.h"
#include "tile_renderer.h"

struct chunk_renderer {
    level_renderer_t const* level_renderer;
    chunk_t const* chunk;
    bool ready;
    GLuint vbo;
    GLuint vao;
    GLuint ebo;
    size_t num_elements;
};

chunk_renderer_t* const chunk_renderer_new(level_renderer_t const* const level_renderer, chunk_t const* const chunk) {
    assert(level_renderer != nullptr);
    assert(chunk != nullptr);

    chunk_renderer_t* const self = malloc(sizeof(chunk_renderer_t));
    assert(self != nullptr);

    self->level_renderer = level_renderer;
    self->chunk = chunk;
    self->ready = false;
    self->num_elements = 0;

    // Set up arrays
    glGenVertexArrays(1, &(self->vao));
    assert(self->vao > 0);
    glBindVertexArray(self->vao);

    glGenBuffers(1, &(self->vbo));
    assert(self->vbo > 0);

    glGenBuffers(1, &(self->ebo));
    assert(self->ebo > 0);

    glBindVertexArray(0);

    return self;
}

void chunk_renderer_delete(chunk_renderer_t* const self) {
    assert(self != nullptr);

    glDeleteBuffers(1, &(self->vbo));
    glDeleteBuffers(1, &(self->ebo));
    glDeleteVertexArrays(1, &(self->vao));

    free(self);
}

bool chunk_renderer_is_ready(chunk_renderer_t const* const self) {
    assert(self != nullptr);

    return self->ready;
}

void chunk_renderer_build(chunk_renderer_t* const self, tessellator_t* const tessellator) {
    assert(self != nullptr);
    assert(tessellator != nullptr);

    // uint64_t start = SDL_GetTicks64();

    tessellator_bind(tessellator, self->vao, self->vbo, 0);

    size_chunks_t chunk_pos[NUM_AXES];
    chunk_get_pos(self->chunk, chunk_pos);

    bool occlusion[NUM_SIDES] = { false };
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t y = 0; y < CHUNK_SIZE; y++) {
            for (size_t z = 0; z < CHUNK_SIZE; z++) {
                for (side_t side = 0; side < NUM_SIDES; side++) {
                    size_t pos[NUM_AXES] = {
                        [AXIS__X] = chunk_pos[AXIS__X] * CHUNK_SIZE + x,
                        [AXIS__Y] = chunk_pos[AXIS__Y] * CHUNK_SIZE + y,
                        [AXIS__Z] = chunk_pos[AXIS__Z] * CHUNK_SIZE + z
                    };
                    occlusion[side] = level_renderer_is_tile_side_occluded(self->level_renderer, pos, side);
                }
                size_t pos[NUM_AXES] = { x, y, z };

                tile_t const* const tile = chunk_get_tile(self->chunk, pos);
                tile_shape_t const tile_shape = chunk_get_tile_shape(self->chunk, pos);

                int const pos_i[NUM_AXES] = { x, y, z };
                tile_renderer_render_tile(tessellator, pos_i, tile, tile_shape, occlusion);
            }
        }
    }

    self->num_elements = tessellator_draw(tessellator);
    self->ready = true;

    // uint64_t end = SDL_GetTicks64();
    // size_chunks_t pos[NUM_AXES];
    // chunk_get_pos(self->chunk, pos);
    // printf("Drew chunk at {%lu, %lu, %lu} - took %lums\n", chunk_pos[AXIS__X], chunk_pos[AXIS__Y], chunk_pos[AXIS__Z], end - start);
}

void chunk_renderer_draw(chunk_renderer_t const* const self) {
    assert(self != nullptr);

    if (self->num_elements == 0) {
        return;
    }

    glBindVertexArray(self->vao);
    glDrawArrays(GL_TRIANGLES, 0, self->num_elements);
    glBindVertexArray(0);
}
