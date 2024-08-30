#pragma once

// Predefines
typedef struct client client_t;

typedef struct font font_t;

typedef enum font_name {
    FONT_NAME__DEFAULT,
    NUM_FONT_NAMES
} font_name_t;

font_t* const font_new(client_t* const client, char const* const resources_path, font_name_t const font_name);

void font_delete(font_t* const self);

void font_draw(font_t const* const self, char const* const text, int const x, int const y);