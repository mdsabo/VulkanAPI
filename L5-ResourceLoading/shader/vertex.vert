#version 450        // GLSL version 4.5

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(vertexPos, 1.0);
    fragColor = color;
}