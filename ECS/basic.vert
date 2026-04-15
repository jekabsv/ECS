#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec4 ourColor;

layout(set = 1, binding = 0) uniform UniformBlock {
    float angle;
} ub;

void main() {
    float c = cos(ub.angle);
    float s = sin(ub.angle);
    vec2 rotated = vec2(
        aPos.x * c - aPos.y * s,
        aPos.x * s + aPos.y * c
    );
    gl_Position = vec4(rotated, 0.0, 1.0);
    ourColor = aColor;
}