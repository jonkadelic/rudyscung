#pragma once

#include "textures.h"

typedef struct font font_t;

font_t* const font_new(textures_t* const textures, char const* const fonts_path, char const* const font_name);

void font_delete(font_t* const font);

void font_draw(font_t const* const font, char const* const text, int const x, int const y);