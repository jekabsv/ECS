#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inVertexColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout(set = 1, binding = 0) uniform EngineData {
    mat4 projection;
    float time;
    float padding[3];
};

layout(set = 1, binding = 1) uniform ObjectData {
    mat4 modelMatrix;
    vec4 colorTint;
};

// This matches your material.uniformVertBufferData (Slot 2)
layout(set = 1, binding = 2) uniform UVTransform {
    vec4 uvRect; // x=offset.x, y=offset.y, z=scale.x, w=scale.y
};

void main()
{
    gl_Position = projection * modelMatrix * vec4(inPos, 0.0, 1.0);
    
    // Transform the 0..1 UVs from the mesh into the correct sprite frame
    // (inUV * scale) + offset
    outUV = (inUV * uvRect.zw) + uvRect.xy;
    
    outColor = inVertexColor * colorTint;
}