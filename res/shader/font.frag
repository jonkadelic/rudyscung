#version 330 core

in vec3 fColor;
in vec2 fTexCoord;

out vec4 FragColor;

uniform sampler2D texture0;

void main() {
    vec4 texColor = texture(texture0, fTexCoord);
    if (texColor.a < 0.1) {
        discard;
    }
    FragColor = texColor * vec4(fColor, 1.0);
}