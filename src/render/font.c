#include "./font.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "../util.h"

typedef struct entry {
    char c;
    size_t x;
    size_t y;
} entry_t;

struct font {
    GLuint tex;
    size_t num_entries;
    entry_t entries[];
};

static size_t read_font_txt_file(char const* const path, char*** const lines);

font_t* const font_new(textures_t* const textures, char const* const fonts_path, char const* const font_name) {
    assert(textures != nullptr);
    assert(fonts_path != nullptr);
    assert(font_name != nullptr);

    char name_buffer[256];
    snprintf(name_buffer, sizeof(name_buffer) / sizeof(name_buffer[0]), "%s/%s.txt", fonts_path, font_name);
    char** lookup_lines = nullptr;
    size_t num_lookup_lines = read_font_txt_file(name_buffer, &lookup_lines);

    for (size_t i = 0; i < num_lookup_lines; i++) {
        printf("%s\n", lookup_lines[i]);
        free(lookup_lines[i]);
    }

    free(lookup_lines);

    return nullptr;
}

void font_delete(font_t* const font) {

}

void font_draw(font_t const* const font, char const* const text, int const x, int const y) {
    
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