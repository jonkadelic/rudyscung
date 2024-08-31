#version 330 core

in vec3 fColor;
in vec2 fTexCoord;

uniform bool hasColor;
uniform bool hasTexture;

out vec4 FragColor;

uniform sampler2D texture0;

void main() {
    if (!hasColor && !hasTexture) {
        discard;
    }
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    if (hasTexture) {
        color *= texture(texture0, fTexCoord);
    }
    if (hasColor) {
        color *= vec4(fColor, 1.0);
    }
    if (color.a < 0.1) {
        discard;
    }
    FragColor = color;
}