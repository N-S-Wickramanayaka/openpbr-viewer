#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aTangent;

layout(location = 0) uniform mat4 uViewProj;
layout(location = 1) uniform mat4 uModel;
layout(location = 2) uniform mat3 uNormalMatrix;   // inverse transpose of uModel's 3x3 part

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vTangent;
out vec2 vUV;

void main()
{
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    vNormal = uNormalMatrix * aNormal;
    vTangent = vec4(mat3(uModel) * aTangent.xyz, aTangent.w);   // tangents follow the surface, not the normal rule
    vUV = aUV;
    gl_Position = uViewProj * world;
}