#include "./shader.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

struct shader {
    GLuint program;
    char const* vertex_source;
    char const* fragment_source;
    bool is_compiled;
};

shader_t* const shader_new(void) {
    shader_t* const self = malloc(sizeof(shader_t));
    self->program = glCreateProgram();
    assert(self->program != 0);
    self->vertex_source = nullptr;
    self->fragment_source = nullptr;
    self->is_compiled = false;

    return self;
}

void shader_attach(shader_t const* const self, shader_type_t const type, char const* const source) {
    assert(self != nullptr);
    assert(source != nullptr);
    assert(self->is_compiled == false);

    GLuint shader_id = glCreateShader(type == SHADER_TYPE__VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    assert(shader_id != 0);

    glShaderSource(shader_id, 1, &source, nullptr);
    glCompileShader(shader_id);

    GLint success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetShaderInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
        printf("Shader compilation failed: %s\n", info_log);
        assert(false);
    }

    glAttachShader(self->program, shader_id);
}

void shader_compile(shader_t* const self) {
    assert(self != nullptr);
    assert(self->is_compiled == false);

    glLinkProgram(self->program);

    GLint success;
    glGetProgramiv(self->program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[512];
        glGetProgramInfoLog(self->program, sizeof(info_log), nullptr, info_log);
        printf("Shader linking failed: %s\n", info_log);
        assert(false);
    }

    self->is_compiled = true;
}

void shader_delete(shader_t* const self) {
    assert(self != nullptr);

    glDeleteProgram(self->program);
    free(self);
}

void shader_bind(shader_t const* const self) {
    assert(self != nullptr);
    assert(self->is_compiled == true);

    glUseProgram(self->program);
}

void shader_put_uniform_mat4x4(shader_t* const shader, char const* const uniform, mat4x4 const value) {
    GLuint location = glGetUniformLocation(shader->program, uniform);
    glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat*) value);
}