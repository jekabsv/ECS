// sprite_instanced.vert
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

struct InstanceGPUData {
    mat4  modelMatrix;
    vec4  colorTint;
    vec4  uniformVert[8];
    vec4  uniformFrag[8];
};

layout(set = 0, binding = 0) readonly buffer InstanceBuffer {
    InstanceGPUData instances[];
};

layout(set = 1, binding = 0) uniform EngineUBO {
    mat4  projection;
    float time;
} engine;

layout(location = 0) out vec4  fragColor;
layout(location = 1) out vec2  fragTexCoord;

void main()
{
    InstanceGPUData inst = instances[gl_InstanceIndex];

    vec4 uvRect = inst.uniformVert[0]; // x=offsetX, y=offsetY, z=scaleX, w=scaleY

    gl_Position  = engine.projection * inst.modelMatrix * vec4(inPosition, 0.0, 1.0);
    fragColor    = inColor * inst.colorTint;
    fragTexCoord = (inTexCoord * uvRect.zw) + uvRect.xy;
}