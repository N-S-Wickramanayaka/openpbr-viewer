#version 460 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vTangent;
in vec2 vUV;

out vec4 fragColor;

// Session 1: show the surface normal as a colour (x -> red, y -> green, z -> blue).
void main()
{
    fragColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}