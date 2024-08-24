#pragma once

#include "shader.h"

typedef struct shaders shaders_t;

shaders_t* const shaders_new(char const* const resources_path);

void shaders_delete(shaders_t* const self);

void shaders_bind(shaders_t* const self, char const* const name);

shader_t* const shaders_get(shaders_t* const self, char const* const name);