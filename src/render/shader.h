#pragma once

#include "lib/linmath.h"

typedef struct shader shader_t;

typedef enum shader_type {
    SHADER_TYPE__VERTEX,
    SHADER_TYPE__FRAGMENT
} shader_type_t;

shader_t* const shader_new(void);

void shader_attach(shader_t const* const self, shader_type_t const type, char const* const source);

void shader_compile(shader_t* const self);

void shader_delete(shader_t* const self);

void shader_bind(shader_t const* const self);

void shader_put_uniform_mat4x4(shader_t* const self, char const* const uniform, mat4x4 const value);