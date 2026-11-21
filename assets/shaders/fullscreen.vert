#version 460 core

// One triangle that covers the whole screen, generated from gl_VertexID: no vertex buffer needed.
void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);   // (0,0), (2,0), (0,2)
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}