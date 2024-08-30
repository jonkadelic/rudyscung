#include "./shaders.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "src/render/shader.h"
#include "src/util/logger.h"
#include "src/util/object_counter.h"

struct shaders {
    shader_t** shaders;
};

static char* read_file(char const* const path);

static char const* const SHADER_NAMES[] = {
    "main",
    "font"
};
static size_t const SHADER_COUNT = sizeof(SHADER_NAMES) / sizeof(SHADER_NAMES[0]);

shaders_t* const shaders_new(char const* const resources_path) {
    assert(resources_path != nullptr);

    shaders_t* const self = malloc(sizeof(shaders_t));
    assert(self != nullptr);

    LOG_DEBUG("shaders_t: initializing...");

    self->shaders = malloc(SHADER_COUNT * sizeof(shader_t*));
    for (size_t i = 0; i < SHADER_COUNT; i++) {
        char name[256] = {0};

        snprintf(name, sizeof(name), "%s/shader/%s.vert", resources_path, SHADER_NAMES[i]);
        char* vertex_source = read_file(name);
        assert(vertex_source != nullptr);

        snprintf(name, sizeof(name), "%s/shader/%s.frag", resources_path, SHADER_NAMES[i]);
        char* fragment_source = read_file(name);
        assert(fragment_source != nullptr);

        shader_t* const shader = shader_new();
        assert(shader != nullptr);

        shader_attach(shader, SHADER_TYPE__VERTEX, vertex_source);
        shader_attach(shader, SHADER_TYPE__FRAGMENT, fragment_source);
        shader_compile(shader);

        self->shaders[i] = shader;

        free(vertex_source);
        vertex_source = nullptr;
        free(fragment_source);
        fragment_source = nullptr;

        LOG_DEBUG("shaders_t: initialized shader \"%s\".", SHADER_NAMES[i]);
    }

    OBJ_CTR_INC(shaders_t);

    return self;
}

void shaders_delete(shaders_t* const self) {
    assert(self != nullptr);

    for (size_t i = 0; i < SHADER_COUNT; i++) {
        if (self->shaders[i] == nullptr) {
            continue;
        }
        shader_delete(self->shaders[i]);
        self->shaders[i] = nullptr;
    }

    free(self->shaders);
    free(self);

    OBJ_CTR_DEC(shaders_t);
}

void shaders_bind(shaders_t* const self, char const* const name) {
    assert(self != nullptr);

    for (size_t i = 0; i < SHADER_COUNT; i++) {
        if (strncmp(SHADER_NAMES[i], name, 16) == 0) {
            shader_bind(self->shaders[i]);
            return;
        }
    }

    assert(false);
}

shader_t* const shaders_get(shaders_t* const self, char const* const name) {
    assert(self != nullptr);

    for (size_t i = 0; i < SHADER_COUNT; i++) {
        if (strncmp(SHADER_NAMES[i], name, 16) == 0) {
            return self->shaders[i];
        }
    }

    assert(false);
}

static char* read_file(char const* const path) {
    FILE* const file = fopen(path, "r");
    if (file == nullptr) {
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    size_t const size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* const buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);

    return buffer;
}