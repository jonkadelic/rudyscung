#pragma once

typedef struct window window_t;

window_t* const window_new(char const* const title, int const width, int const height);

void window_delete(window_t* const self);

void window_swap(window_t const* const self);
