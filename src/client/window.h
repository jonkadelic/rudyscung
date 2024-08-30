#pragma once

#include <stddef.h>

typedef struct window window_t;

window_t* const window_new(char const* const title, int const width, int const height);

void window_delete(window_t* const self);

void window_swap(window_t const* const self);

void window_get_size(window_t const* const self, size_t size[2]);

void window_handle_resize(window_t* const self);

float window_get_gui_scale(window_t const* const self);

void window_set_gui_scale(window_t* const self, float const scale);
