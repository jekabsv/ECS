#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(set = 1, binding = 0) uniform ViewProj {
    mat4 viewProj;
} vp;

layout(set = 1, binding = 1) uniform Object {
    mat4 model;
    vec4 colorTint;
} obj;

layout(set = 1, binding = 2) uniform CustomVert {
    float offsetX;
    float offsetY;
    float padding[2];
} custom;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = obj.model * vec4(inPosition, 0.0, 1.0);
    worldPos.x += custom.offsetX;
    worldPos.y += custom.offsetY;
    gl_Position = vp.viewProj * worldPos;
    fragColor = inColor * obj.colorTint;
    fragTexCoord = inTexCoord;
}