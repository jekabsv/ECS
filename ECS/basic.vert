#version 450
void main() {
    // Hardcoded triangle in NDC space
    vec2 pos[3] = vec2[3](vec2(0.0, 0.5), vec2(0.5, -0.5), vec2(-0.5, -0.5));
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
}