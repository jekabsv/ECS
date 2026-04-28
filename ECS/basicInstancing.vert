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

// Storage buffer → Set 0 (SDL_GPU vertex storage buf slot 0)
layout(set = 0, binding = 0) readonly buffer InstanceBuffer {
    InstanceGPUData instances[];
};

// Uniform buffer → Set 1 (SDL_GPU vertex uniform buf slot 0)
layout(set = 1, binding = 0) uniform EngineUBO {
    mat4  projection;
    float time;
} engine;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    InstanceGPUData inst = instances[gl_InstanceIndex];
    gl_Position  = engine.projection * inst.modelMatrix * vec4(inPosition, 0.0, 1.0);
    fragColor    = inColor * inst.colorTint;
    fragTexCoord = inTexCoord;
}