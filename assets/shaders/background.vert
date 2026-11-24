#version 460 core

out vec2 vNdc;

// A full-screen triangle placed on the far plane (z = w), so with depth test LEQUAL it only covers
// pixels where nothing else was drawn.
void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);   // (0,0), (2,0), (0,2)
    vNdc = p * 2.0 - 1.0;
    gl_Position = vec4(vNdc, 1.0, 1.0);
}