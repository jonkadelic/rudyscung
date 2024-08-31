#include "line.h"

#include <assert.h>

#include "src/render/gl.h"
#include "src/render/tessellator.h"
#include "src/world/side.h"

static struct {
    tessellator_t* tessellator;
    GLuint vao;
    GLuint vbo;
} state = {
    .tessellator = nullptr,
    .vao = 0,
    .vbo = 0
};

void line_init(void) {
    state.tessellator = tessellator_new();

    glGenVertexArrays(1, &(state.vao));
    assert(state.vao > 0);
    glBindVertexArray(state.vao);

    glGenBuffers(1, &(state.vbo));
    assert(state.vbo > 0);

    glBindVertexArray(0);

    tessellator_bind(state.tessellator, state.vao, state.vbo, 0);
}

void line_cleanup(void) {
    tessellator_delete(state.tessellator);

    glDeleteBuffers(1, &(state.vbo));
    glDeleteVertexArrays(1, &(state.vao));
}

void line_render(float const a[NUM_AXES], float const b[NUM_AXES], float const width, float const color[3]) {
    assert(width > 0.0f);

    glLineWidth(width);

    tessellator_buffer_vc(state.tessellator, a[AXIS__X], a[AXIS__Y], a[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, b[AXIS__X], b[AXIS__Y], b[AXIS__Z], color[0], color[1], color[2]);

    size_t const num_elements = tessellator_draw(state.tessellator);

    glBindVertexArray(state.vao);

    glDrawArrays(GL_LINES, 0, num_elements);

    glBindVertexArray(0);
}

void line_render_box(float const min[NUM_AXES], float const max[NUM_AXES], float const width, float const color[3]) {
    assert(width > 0.0f);

    glLineWidth(width);

    tessellator_buffer_vc(state.tessellator, min[AXIS__X], min[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, max[AXIS__X], min[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, max[AXIS__X], min[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, max[AXIS__X], max[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, max[AXIS__X], max[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, min[AXIS__X], max[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, min[AXIS__X], max[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, min[AXIS__X], min[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, min[AXIS__X], min[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, max[AXIS__X], min[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, max[AXIS__X], min[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, max[AXIS__X], max[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, max[AXIS__X], max[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, min[AXIS__X], max[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, min[AXIS__X], max[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, min[AXIS__X], min[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, min[AXIS__X], min[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, min[AXIS__X], min[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, max[AXIS__X], min[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, max[AXIS__X], min[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, max[AXIS__X], max[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, max[AXIS__X], max[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    tessellator_buffer_vc(state.tessellator, min[AXIS__X], max[AXIS__Y], min[AXIS__Z], color[0], color[1], color[2]);
    tessellator_buffer_vc(state.tessellator, min[AXIS__X], max[AXIS__Y], max[AXIS__Z], color[0], color[1], color[2]);

    size_t const num_elements = tessellator_draw(state.tessellator);

    glBindVertexArray(state.vao);

    glDrawArrays(GL_LINES, 0, num_elements);

    glBindVertexArray(0);
}
