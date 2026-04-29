// sprite_instanced.frag
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D spriteTexture;

void main()
{
    vec4 tex = texture(spriteTexture, fragTexCoord);
    vec4 col = tex * fragColor;
    if (col.a < 0.01)
        discard;
    outColor = col;
}