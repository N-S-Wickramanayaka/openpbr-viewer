#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

layout(location = 0) uniform mat4 uModelViewProj;

out vec2 vUV;

void main()
{
    vUV = aUV;
    gl_Position = uModelViewProj * vec4(aPosition, 1.0);
}