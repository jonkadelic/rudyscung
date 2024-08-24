#include "./chunk_renderer.h"

#include <assert.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL_timer.h>

#include "tessellator.h"
#include "tile_renderer.h"

struct chunk_renderer {
    chunk_t const* chunk;
    bool ready;
    GLuint vbo;
    GLuint vao;
    GLuint ebo;
    size_t num_elements;
};

chunk_renderer_t* const chunk_renderer_new(chunk_t const* const chunk) {
    assert(chunk != nullptr);

    chunk_renderer_t* const chunk_renderer = malloc(sizeof(chunk_renderer_t));
    assert(chunk_renderer != nullptr);

    chunk_renderer->chunk = chunk;
    chunk_renderer->ready = false;
    chunk_renderer->num_elements = 0;

    // Set up arrays
    glGenVertexArrays(1, &(chunk_renderer->vao));
    assert(chunk_renderer->vao > 0);
    glBindVertexArray(chunk_renderer->vao);

    glGenBuffers(1, &(chunk_renderer->vbo));
    assert(chunk_renderer->vbo > 0);

    glGenBuffers(1, &(chunk_renderer->ebo));
    assert(chunk_renderer->ebo > 0);

    glBindVertexArray(0);

    return chunk_renderer;
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

    bool occlusion[NUM_SIDES] = { false };
    for (size_t x = 0; x < CHUNK_SIZE; x++) {
        for (size_t y = 0; y < CHUNK_SIZE; y++) {
            for (size_t z = 0; z < CHUNK_SIZE; z++) {
                for (side_t side = 0; side < NUM_SIDES; side++) {
                    occlusion[side] = chunk_is_tile_side_occluded(self->chunk, x, y, z, side);
                }

                tile_t const* const tile = chunk_get_tile(self->chunk, x, y, z);
                tile_shape_t const tile_shape = chunk_get_tile_shape(self->chunk, x, y, z);

                tile_renderer_render_tile(tessellator, x, y, z, tile, tile_shape, occlusion);
            }
        }
    }

    self->num_elements = tessellator_draw(tessellator);
    self->ready = true;

    // uint64_t end = SDL_GetTicks64();
    // size_chunks_t pos[3];
    // chunk_get_pos(self->chunk, pos);
    // printf("Drew chunk at {%lu, %lu, %lu} - took %lums\n", pos[0], pos[1], pos[2], end - start);
}

void chunk_renderer_draw(chunk_renderer_t const* const self) {
    assert(self != nullptr);

    glBindVertexArray(self->vao);
    glDrawArrays(GL_TRIANGLES, 0, self->num_elements);
    glBindVertexArray(0);
}
