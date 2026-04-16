#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outFragColor;

// Note: Ensure your 'set' and 'binding' here match your BindMaterialTextures logic
layout(set = 2, binding = 0) uniform sampler2D spriteTexture;

void main()
{
    vec4 tex = texture(spriteTexture, inUV);
    vec4 col = tex * inColor;

    // Standard alpha testing to prevent writing to depth buffer for nearly invisible pixels
    if (col.a < 0.01)
        discard;

    outFragColor = col;
}