#pragma once

#include "shader.h"

void shaders_init(char const* const resources_path);

void shaders_cleanup(void);

void shaders_bind(char const* const name);

shader_t* const shaders_get(char const* const name);