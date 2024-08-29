#include "./tessellator.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "src/util/gl.h"

#define MAX_ENTRIES (1024 * 128)

typedef struct vertex {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    float u;
    float v;
} vertex_t;

typedef struct entry {
    bool is_index;
    union {
        vertex_t vertex;
        size_t index;
    };
} entry_t;

struct tessellator {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    entry_t entries[MAX_ENTRIES];
    size_t entry_count;
    size_t vertex_count;
    bool has_color;
    bool has_tex;
};

static bool entries_equal(entry_t const* const a, entry_t const* const b, bool const has_color, bool const has_tex);
static void put_entry(tessellator_t* const tessellator, entry_t const* const entry);

tessellator_t* const tessellator_new(void) {
    tessellator_t* self = malloc(sizeof(tessellator_t));
    assert(self != nullptr);

    tessellator_reset(self);

    return self;
}

void tessellator_delete(tessellator_t* const self) {
    assert(self != nullptr);

    free(self);
}

void tessellator_reset(tessellator_t* const self) {
    assert(self != nullptr);

    self->entry_count = 0;
    self->vertex_count = 0;
    self->has_color = false;
    self->has_tex = false;
}

void tessellator_bind(tessellator_t* const self, GLuint vao, GLuint vbo, GLuint ebo) {
    assert(self != nullptr);
    assert(vao > 0);
    assert(vbo > 0);

    self->vao = vao;
    self->vbo = vbo;
    self->ebo = ebo;
}

size_t tessellator_draw(tessellator_t* const self) {
    assert(self != nullptr);
    assert(self->vao > 0);
    assert(self->vbo > 0);

    size_t floats_per_row = 3;
    size_t color_offset = 3;
    size_t tex_offset = 3;
    if (self->has_color) {
        floats_per_row += 3;
        tex_offset += 3;
    }
    if (self->has_tex) {
        floats_per_row += 2;
    }

    GLfloat* vertices = malloc(sizeof(GLfloat) * floats_per_row * self->vertex_count);
    assert(vertices != nullptr);
    GLuint* indices = malloc(sizeof(GLuint) * self->entry_count);
    assert(indices != nullptr);

    size_t acc = 0;
    for (size_t i = 0; i < self->entry_count; i++) {
        entry_t const* const entry = &(self->entries[i]);
        if (entry->is_index) {
            // Add index
            indices[i] = entry->index;
        } else {
            // Add index
            indices[i] = acc;

            // Add vertex
            size_t index = acc * floats_per_row;
            vertices[index + 0] = entry->vertex.x;
            vertices[index + 1] = entry->vertex.y;
            vertices[index + 2] = entry->vertex.z;

            size_t o = 3;

            if (self->has_color) {
                vertices[index + o + 0] = entry->vertex.r;
                vertices[index + o + 1] = entry->vertex.g;
                vertices[index + o + 2] = entry->vertex.b;

                o += 3;
            }
            if (self->has_tex) {
                vertices[index + o + 0] = entry->vertex.u;
                vertices[index + o + 1] = entry->vertex.v;

                o += 2;
            }

            acc++;
        }
    }

    // Buffer vertices
    glBindVertexArray(self->vao);

    glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * floats_per_row * self->vertex_count, vertices, GL_STATIC_DRAW);

    // Set up position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, floats_per_row * sizeof(GLfloat), (void*)(0 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);

    // Set up color attribute
    if (self->has_color) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, floats_per_row * sizeof(GLfloat), (void*)(color_offset * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
    } else {
        glDisableVertexAttribArray(1);
    }

    // Set up tex attribute
    if (self->has_tex) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, floats_per_row * sizeof(GLfloat), (void*)(tex_offset * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
    } else {
        glDisableVertexAttribArray(2);
    }

    // Buffer elements
    if (self->ebo > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * self->entry_count, indices, GL_STATIC_DRAW);
    }

    glBindVertexArray(0);

    free(vertices);
    free(indices);

    size_t entry_count = self->entry_count;

    tessellator_reset(self);

    return entry_count;
}

void tessellator_buffer_v(tessellator_t* const self, float x, float y, float z) {
    assert(self != nullptr);
    assert(self->entry_count < MAX_ENTRIES);
    assert(self->has_color == false);
    assert(self->has_tex == false);

    entry_t entry = {
        .is_index = false,
        .vertex = {
            .x = x,
            .y = y,
            .z = z
        }
    };

    put_entry(self, &entry);
}

void tessellator_buffer_vc(tessellator_t* const self, float x, float y, float z, float r, float g, float b) {
    assert(self != nullptr);
    assert(self->entry_count < MAX_ENTRIES);
    assert((self->has_color == false && self->entry_count == 0) || (self->has_color == true && self->entry_count > 0));
    assert(self->has_tex == false);

    entry_t entry = {
        .is_index = false,
        .vertex = {
            .x = x,
            .y = y,
            .z = z,
            .r = r,
            .g = g,
            .b = b,
        }
    };

    put_entry(self, &entry);

    self->has_color = true;
}

void tessellator_buffer_vt(tessellator_t* const self, float x, float y, float z, float u, float v) {
    assert(self != nullptr);
    assert(self->entry_count < MAX_ENTRIES);
    assert(self->has_color == false);
    assert((self->has_tex == false && self->entry_count == 0) || (self->has_tex == true && self->entry_count > 0));

    entry_t entry = {
        .is_index = false,
        .vertex = {
            .x = x,
            .y = y,
            .z = z,
            .u = u,
            .v = v
        }
    };

    put_entry(self, &entry);

    self->has_tex = true;
}

void tessellator_buffer_vct(tessellator_t* const self, float x, float y, float z, float r, float g, float b, float u, float v) {
    assert(self != nullptr);
    assert(self->entry_count < MAX_ENTRIES);
    assert((self->has_color == false && self->entry_count == 0) || (self->has_color == true && self->entry_count > 0));
    assert((self->has_tex == false && self->entry_count == 0) || (self->has_tex == true && self->entry_count > 0));

    entry_t entry = {
        .is_index = false,
        .vertex = {
            .x = x,
            .y = y,
            .z = z,
            .r = r,
            .g = g,
            .b = b,
            .u = u,
            .v = v
        }
    };

    put_entry(self, &entry);

    self->has_color = true;
    self->has_tex = true;
}

static bool entries_equal(entry_t const* const a, entry_t const* const b, bool const has_color, bool const has_tex) {
    assert(a != nullptr);
    assert(b != nullptr);
    
    if (a != b) {
        if (a->is_index != b->is_index) {
            return false;
        }
        if (a->is_index) {
            return a->index == b->index;
        } else {
            if ((a->vertex.x != b->vertex.x) || (a->vertex.y != b->vertex.y) || (a->vertex.z != b->vertex.z)) {
                return false;
            }
            if (has_color) {
                if ((a->vertex.r != b->vertex.r) || (a->vertex.g != b->vertex.g) || (a->vertex.b != b->vertex.b)) {
                    return false;
                }
            }
            if (has_tex) {
                if ((a->vertex.u != b->vertex.u) || (a->vertex.v != b->vertex.v)) {
                    return false;
                }
            }
        }
    }

    return true;
}

static void put_entry(tessellator_t* const self, entry_t const* const entry) {
    assert(self != nullptr);
    assert(entry != nullptr);

    entry_t e;
    memcpy(&e, entry, sizeof(entry_t));

    if (self->ebo > 0) {
        size_t skipped = 0;
        for (size_t i = 0; i < self->entry_count; i++) {
            entry_t const* const ie = &(self->entries[i]);
            if (ie->is_index) {
                skipped++;
            } else {
                if (entries_equal(&e, ie, self->has_color, self->has_tex)) {
                    e.is_index = true;
                    e.index = i - skipped;
                    break;
                }
            }
        }
    }

    memcpy(&(self->entries[self->entry_count]), &e, sizeof(entry_t));
    self->entry_count++;

    if (!e.is_index) {
        self->vertex_count++;
    }
}