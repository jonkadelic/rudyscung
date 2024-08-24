#pragma once

#include "render/font.h"
#include "render/shaders.h"
#include "render/textures.h"
#include "window.h"

typedef struct rudyscung rudyscung_t;

rudyscung_t* const rudyscung_new(char const* const resources_path);

void rudyscung_delete(rudyscung_t* const self);

void rudyscung_run(rudyscung_t* const self);

window_t* const rudyscung_get_window(rudyscung_t* const self);

shaders_t* const rudyscung_get_shaders(rudyscung_t* const self);

textures_t* const rudyscung_get_textures(rudyscung_t* const self);

font_t* const rudyscung_get_font(rudyscung_t* const self);