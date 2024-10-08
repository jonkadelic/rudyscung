#pragma once

#include <stddef.h>

#include "src/render/gl.h"

typedef struct tessellator tessellator_t;

tessellator_t* const tessellator_new(void);

void tessellator_delete(tessellator_t* const self);

void tessellator_reset(tessellator_t* const self);
void tessellator_bind(tessellator_t* const self, GLuint vao, GLuint vbo, GLuint ebo);
void tessellator_has(tessellator_t* const self, bool has_color[1], bool has_tex[1]);
size_t tessellator_draw(tessellator_t* const self);

void tessellator_buffer_v(tessellator_t* const self, float x, float y, float z);
void tessellator_buffer_vc(tessellator_t* const self, float x, float y, float z, float r, float g, float b);
void tessellator_buffer_vt(tessellator_t* const self, float x, float y, float z, float u, float v);
void tessellator_buffer_vct(tessellator_t* const self, float x, float y, float z, float r, float g, float b, float u, float v);