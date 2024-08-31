#pragma once

#include "src/render/font.h"
#include "src/render/shaders.h"
#include "src/render/textures.h"
#include "src/render/view_type.h"
#include "src/client/window.h"

typedef struct client client_t;

client_t* const client_new(char const* const resources_path);

void client_delete(client_t* const self);

void client_run(client_t* const self);

window_t* const client_get_window(client_t* const self);

shaders_t* const client_get_shaders(client_t* const self);

textures_t* const client_get_textures(client_t* const self);

font_t* const client_get_font(client_t* const self);

level_t* const client_get_level(client_t* const self);

entity_t const client_get_player(client_t* const self);

view_type_t* const client_get_view_type(client_t* const self);

void client_set_view_type(client_t* const self, view_type_t* const view_type);