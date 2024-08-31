#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec2 vTexCoord;

uniform bool hasColor;
uniform bool hasTexture;

out vec3 fColor;
out vec2 fTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(vPos, 1.0);
    if (hasColor) {
        fColor = vColor;
    } else {
        fColor = vec3(1.0, 1.0, 1.0);
    }
    if (hasTexture) {
        fTexCoord = vTexCoord;
    } else {
        fTexCoord = vec2(0.0, 0.0);
    }
}