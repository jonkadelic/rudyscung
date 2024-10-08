#include "./font.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/cglm.h>

#include "src/render/gl.h"
#include "lib/stb_image.h"
#include "src/client/client.h"
#include "src/render/shader.h"
#include "src/render/tessellator.h"
#include "src/render/textures.h"
#include "src/render/shaders.h"
#include "src/util/logger.h"
#include "src/util/object_counter.h"

#define MAX_ENTRIES 128

struct {
    texture_name_t const texture_name;
    char const* const name;
} const FONT_NAME_INFO[NUM_FONT_NAMES] = {
    [FONT_NAME__DEFAULT] = {
        .texture_name = TEXTURE_NAME__FONT_DEFAULT,
        .name = "default"
    }
};

typedef struct entry {
    bool exists;
    size_t x;
    size_t y;
    size_t width;
} entry_t;

struct font {
    client_t* client;

    GLuint tex;
    size_t char_size_px;
    size_t font_size_px;
    entry_t entries[MAX_ENTRIES];

    tessellator_t* tessellator;
    GLuint vbo;
    GLuint vao;
};

static size_t read_font_txt_file(char const* const path, char*** const lines);

font_t* const font_new(client_t* const client, char const* const resources_path, font_name_t const font_name) {
    assert(client != nullptr);
    assert(resources_path != nullptr);
    assert(font_name >= 0 && font_name < NUM_FONT_NAMES);

    auto font_name_info = &(FONT_NAME_INFO[font_name]);

    char name_buffer[256];
    snprintf(name_buffer, sizeof(name_buffer) / sizeof(name_buffer[0]), "%s/font/%s.txt", resources_path, font_name_info->name);
    char** lookup_lines = nullptr;
    size_t num_lookup_lines = read_font_txt_file(name_buffer, &lookup_lines);
    size_t lookup_line_len = strlen(lookup_lines[0]);

    font_t* const self = malloc(sizeof(font_t) + (sizeof(entry_t)));
    assert(self != nullptr);

    self->client = client;

    // Blank entries
    for (size_t i = 0; i < MAX_ENTRIES; i++) {
        self->entries[i].exists = false;
    }

    // Get texture
    textures_t* const textures = client_get_textures(self->client);
    texture_t const* const texture = textures_get_texture(textures, font_name_info->texture_name);
    self->tex = texture->name;

    self->font_size_px = texture->size[0];
    self->char_size_px = self->font_size_px / lookup_line_len;

    for (size_t y = 0; y < num_lookup_lines; y++) {
        for (size_t x = 0; x < lookup_line_len; x++) {
            uint8_t c = lookup_lines[y][x];
            if (self->entries[c].exists) continue;
            size_t max_px = 0;
            for (size_t px = 0; px < self->char_size_px; px++) {
                for (size_t py = 0; py < self->char_size_px; py++) {
                    uint32_t p = texture_get_pixel(texture, (size_t[2]) { x * self->char_size_px + px, y * self->char_size_px + py });
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

    OBJ_CTR_INC(font_t);

    return self;
}

void font_delete(font_t* const self) {
    assert(self != nullptr);

    glDeleteBuffers(1, &(self->vbo));
    glDeleteVertexArrays(1, &(self->vao));

    tessellator_delete(self->tessellator);

    free(self);

    OBJ_CTR_DEC(font_t);
}

void font_draw(font_t const* const self, char const* const text, int const x, int const y) {
    assert(self != nullptr);
    assert(text != nullptr);

    size_t text_len = strlen(text);
    if (text_len == 0) return;

    size_t xo = 0;
    for (size_t i = 0; i < text_len; i++) {
        uint8_t c = text[i];
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

    shaders_t* const shaders = client_get_shaders(self->client);
    shader_t* const shader = shaders_get(shaders, "font");
    shader_bind(shader);

    window_t* const window = client_get_window(self->client);
    size_t window_size[2];
    window_get_size(window, window_size);
    float gui_scale = window_get_gui_scale(window);

    mat4 mat_proj;
    glm_mat4_identity(mat_proj);
    glm_ortho(0, window_size[0] / gui_scale, window_size[1] / gui_scale, 0, 100, 300, mat_proj);
    shader_put_uniform_mat4(shader, "projection", mat_proj);

    mat4 mat_model;
    glm_mat4_identity(mat_model);
    glm_translate(mat_model, (vec3) { 0, 0, -200 });
    shader_put_uniform_mat4(shader, "model", mat_model);

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
