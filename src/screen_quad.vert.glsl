#version 430 core

// based on https://stackoverflow.com/a/59739538

out vec2 uv;

const vec2 vertices[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));

void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
    uv = 0.5 * gl_Position.xy + vec2(0.5);
}