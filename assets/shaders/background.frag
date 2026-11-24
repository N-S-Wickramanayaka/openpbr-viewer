#version 460 core

in vec2 vNdc;

layout(binding = 0) uniform sampler2D uPanorama;

layout(location = 0) uniform mat4 uInvViewRotProj;   // inverse of (projection x view rotation)
layout(location = 1) uniform mat3 uEnvRotation;
layout(location = 2) uniform float uIntensity;

out vec4 fragColor;

const float PI = 3.14159265359;

// Where a direction lands in an equirectangular panorama stored top row first. Same convention as the
// Khronos glTF Sample Renderer: v = 0 at the top of the sky; u runs around the horizon starting at -X.
vec2 directionToEquirectUV(vec3 d)
{
    return vec2(0.5 + 0.5 * atan(d.z, d.x) / PI, acos(clamp(d.y, -1.0, 1.0)) / PI);
}

void main()
{
    // The point on the far plane under this pixel, seen from a camera at the origin, gives the view direction.
    vec4 farPoint = uInvViewRotProj * vec4(vNdc, 1.0, 1.0);
    vec3 direction = uEnvRotation * normalize(farPoint.xyz / farPoint.w);
    fragColor = vec4(textureLod(uPanorama, directionToEquirectUV(direction), 0.0).rgb * uIntensity, 1.0);
}