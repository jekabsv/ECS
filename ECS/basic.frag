#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// Slot 0 — custom per-material fragment data
layout(set = 3, binding = 0) uniform CustomFrag {
    vec4 colorMul;
} customFrag;

void main() {
    outColor = fragColor * customFrag.colorMul;
}