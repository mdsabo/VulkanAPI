#version 450

layout(location = 0) out vec4 outColor; // connects to output

layout(location = 0) in vec3 fragColor;

void main() {
    outColor = vec4(fragColor, 1.0f);
}