#include "./font.h"

#include <SDL2/SDL_pixels.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "../rudyscung.h"
#include "shader.h"
#include "tessellator.h"
#include "textures.h"
#include "shaders.h"
#include "../linmath.h"
#include "../util/logger.h"

#define MAX_ENTRIES 128

typedef struct entry {
    bool exists;
    size_t x;
    size_t y;
    size_t width;
} entry_t;

struct font {
    rudyscung_t* rudyscung;

    GLuint tex;
    size_t char_size_px;
    size_t font_size_px;
    entry_t entries[MAX_ENTRIES];

    tessellator_t* tessellator;
    GLuint vbo;
    GLuint vao;
};

static size_t read_font_txt_file(char const* const path, char*** const lines);
static uint32_t get_pixel(SDL_Surface const* const surface, size_t const x, size_t const y);

font_t* const font_new(rudyscung_t* const rudyscung, char const* const resources_path, char const* const font_name) {
    assert(rudyscung != nullptr);
    assert(resources_path != nullptr);
    assert(font_name != nullptr);

    LOG_DEBUG("font_t: initializing \"%s\".", font_name);

    char name_buffer[256];
    snprintf(name_buffer, sizeof(name_buffer) / sizeof(name_buffer[0]), "%s/font/%s.txt", resources_path, font_name);
    char** lookup_lines = nullptr;
    size_t num_lookup_lines = read_font_txt_file(name_buffer, &lookup_lines);
    size_t lookup_line_len = strlen(lookup_lines[0]);

    font_t* const self = malloc(sizeof(font_t) + (sizeof(entry_t)));
    assert(self != nullptr);

    self->rudyscung = rudyscung;

    // Blank entries
    for (size_t i = 0; i < MAX_ENTRIES; i++) {
        self->entries[i].exists = false;
    }

    snprintf(name_buffer, sizeof(name_buffer) / sizeof(name_buffer[0]), "%s/font/%s.png", resources_path, font_name);
    SDL_Surface* surface = IMG_Load(name_buffer);
    assert(surface != nullptr);

    self->font_size_px = surface->w;
    self->char_size_px = self->font_size_px / lookup_line_len;

    for (size_t y = 0; y < num_lookup_lines; y++) {
        for (size_t x = 0; x < lookup_line_len; x++) {
            char c = lookup_lines[y][x];
            if (self->entries[c].exists) continue;
            size_t max_px = 0;
            for (size_t px = 0; px < self->char_size_px; px++) {
                for (size_t py = 0; py < self->char_size_px; py++) {
                    uint32_t p = get_pixel(surface, x * self->char_size_px + px, y * self->char_size_px + py);
                    if ((p & 0xFF000000) != 0x00000000) {
                        max_px = px;
                        break;
                    }
                }
            }
            self->entries[c].exists = true;
            self->entries[c].width = max_px + 1;
            self->entries[c].x = x * self->char_size_px;
            self->entries[c].y = y * self->char_size_px;
        }
    }

    // Get texture
    textures_t* const textures = rudyscung_get_textures(self->rudyscung);
    snprintf(name_buffer, sizeof(name_buffer) / sizeof(name_buffer[0]), "/font/%s.png", font_name);
    self->tex = textures_get_texture_by_path(textures, name_buffer);

    // Delete surface
    SDL_FreeSurface(surface);

    // Free lookup
    for (size_t i = 0; i < num_lookup_lines; i++) {
        free(lookup_lines[i]);
    }
    free(lookup_lines);

    // Set up arrays
    glGenVertexArrays(1, &(self->vao));
    assert(self->vao > 0);

    glGenBuffers(1, &(self->vbo));
    assert(self->vbo > 0);

    // Set up tessellator
    self->tessellator = tessellator_new();
    tessellator_bind(self->tessellator, self->vao, self->vbo, 0);

    LOG_DEBUG("font_t: initialized \"%s\".", font_name);

    return self;
}

void font_delete(font_t* const self) {
    assert(self != nullptr);

    glDeleteBuffers(1, &(self->vbo));
    glDeleteVertexArrays(1, &(self->vao));

    free(self);

    LOG_DEBUG("font_t: deleted.");
}

void font_draw(font_t const* const self, char const* const text, int const x, int const y) {
    assert(self != nullptr);
    assert(text != nullptr);

    size_t text_len = strlen(text);
    if (text_len == 0) return;

    size_t xo = 0;
    for (size_t i = 0; i < text_len; i++) {
        char c = text[i];
        entry_t const* const entry = &(self->entries[c]);
        if (!self->entries[c].exists) {
            c = '?';
        }
        float u_min = (float)(entry->x) / self->font_size_px;
        float u_max = u_min + ((float)(entry->width) / self->font_size_px);
        float v_min = (float)(entry->y) / self->font_size_px;
        float v_max = v_min + ((float)(self->char_size_px) / self->font_size_px);

        #define Z_POS 0

        tessellator_buffer_vct(self->tessellator, x + xo, y, Z_POS, 1.0f, 1.0f, 1.0f, u_min, v_min);
        tessellator_buffer_vct(self->tessellator, x + xo + entry->width, y + self->char_size_px, Z_POS, 1.0f, 1.0f, 1.0f, u_max, v_max);
        tessellator_buffer_vct(self->tessellator, x + xo + entry->width, y, Z_POS, 1.0f, 1.0f, 1.0f, u_max, v_min);

        tessellator_buffer_vct(self->tessellator, x + xo, y, Z_POS, 1.0f, 1.0f, 1.0f, u_min, v_min);
        tessellator_buffer_vct(self->tessellator, x + xo, y + self->char_size_px, Z_POS, 1.0f, 1.0f, 1.0f, u_min, v_max);
        tessellator_buffer_vct(self->tessellator, x + xo + entry->width, y + self->char_size_px, Z_POS, 1.0f, 1.0f, 1.0f, u_max, v_max);

        xo += entry->width + 1;        
    }

    size_t elements = tessellator_draw(self->tessellator);

    shaders_t* const shaders = rudyscung_get_shaders(self->rudyscung);
    shader_t* const shader = shaders_get(shaders, "font");
    shader_bind(shader);

    window_t* const window = rudyscung_get_window(self->rudyscung);
    size_t window_size[2];
    window_get_size(window, window_size);
    float gui_scale = window_get_gui_scale(window);

    mat4x4 mat_proj;
    mat4x4_identity(mat_proj);
    mat4x4_ortho(mat_proj, 0, window_size[0] / gui_scale, window_size[1] / gui_scale, 0, 100, 300);
    shader_put_uniform_mat4x4(shader, "projection", mat_proj);

    mat4x4 mat_model;
    mat4x4_identity(mat_model);
    mat4x4_translate_in_place(mat_model, 0, 0, -200);
    shader_put_uniform_mat4x4(shader, "model", mat_model);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBindTexture(GL_TEXTURE_2D, self->tex);

    glBindVertexArray(self->vao);
    glDrawArrays(GL_TRIANGLES, 0, elements);
    glBindVertexArray(0);

    glDisable(GL_CULL_FACE);
}

static size_t read_font_txt_file(char const* const path, char*** const lines) {
    assert(path != nullptr);
    assert(lines != nullptr);

    FILE* fp  = fopen(path, "r");
    assert(fp != nullptr);

    fseek(fp, 0, SEEK_END);
    size_t const size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* const buffer = malloc(size + 1);
    assert(buffer != nullptr);
    fread(buffer, 1, size, fp);
    fclose(fp);
    buffer[size] = '\0';
    
    size_t line_length = 0;
    size_t num_lines = 1;
    for (size_t i = 0; i < size; i++) {
        char c = buffer[i];
        if (c == '\n') {
            num_lines++;
        }
        if (num_lines == 1) {
            line_length++;
        }
    }

    // Allocate lines
    *lines = malloc(sizeof(char*) * num_lines);
    assert(*lines != nullptr);
    for (size_t i = 0; i < num_lines; i++) {
        (*lines)[i] = malloc(sizeof(char) * (line_length + 1));
        assert((*lines)[i] != nullptr);
        for (size_t j = 0; j < line_length; j++) {
            (*lines)[i][j] = buffer[(line_length + 1) * i + j];
        }
        (*lines)[i][line_length] = '\0';
    }

    free(buffer);

    return num_lines;
}

static uint32_t get_pixel(SDL_Surface const* const surface, size_t const x, size_t const y) {
    assert(surface != nullptr);

    if (surface->format->BytesPerPixel == 1) {
        uint8_t lookup = ((uint8_t*) surface->pixels)[y * surface->w + x];
        SDL_Color const* const c = &(surface->format->palette->colors[lookup]);
        return (c->a << 24) | (c->r << 16) | (c->g << 8) | (c->b);
    }

    return ((uint32_t*) surface->pixels)[y * surface->w + x];
}