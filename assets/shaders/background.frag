#version 460 core

in vec2 vNdc;

layout(binding = 0) uniform samplerCube uEnvironment;

layout(location = 0) uniform mat4 uInvViewRotProj;   // inverse of (projection x view rotation)
layout(location = 1) uniform mat3 uEnvRotation;
layout(location = 2) uniform float uIntensity;

out vec4 fragColor;

void main()
{
    // The point on the far plane under this pixel, seen from a camera at the origin, gives the view direction.
    vec4 farPoint = uInvViewRotProj * vec4(vNdc, 1.0, 1.0);
    vec3 direction = uEnvRotation * normalize(farPoint.xyz / farPoint.w);
    fragColor = vec4(textureLod(uEnvironment, direction, 0.0).rgb * uIntensity, 1.0);
}