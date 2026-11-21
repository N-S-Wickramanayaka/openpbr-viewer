#version 460 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vTangent;
in vec2 vUV;

layout(binding = 0) uniform sampler2D uBaseColorTex;          // sRGB texture
layout(binding = 1) uniform sampler2D uMetallicRoughnessTex;  // linear: G = roughness, B = metallic
layout(binding = 2) uniform sampler2D uNormalTex;             // linear, tangent space
layout(binding = 3) uniform sampler2D uOcclusionTex;          // linear: R = occlusion
layout(binding = 4) uniform sampler2D uEmissiveTex;           // sRGB texture

layout(location = 3) uniform vec4 uBaseColorFactor;
layout(location = 4) uniform float uMetallicFactor;
layout(location = 5) uniform float uRoughnessFactor;
layout(location = 6) uniform vec3 uEmissiveFactor;
layout(location = 7) uniform float uNormalScale;
layout(location = 8) uniform float uOcclusionStrength;
layout(location = 9) uniform int uDebugView;

out vec4 fragColor;

// Debug views, in the same order as the ImGui combo
const int VIEW_LIT = 0, VIEW_BASE_COLOR = 1, VIEW_METALLIC = 2, VIEW_ROUGHNESS = 3, VIEW_NORMAL = 4,
          VIEW_OCCLUSION = 5, VIEW_EMISSIVE = 6, VIEW_UV = 7;

vec3 shadingNormal()
{
    vec3 n = normalize(vNormal);
    vec3 t = normalize(vTangent.xyz - n * dot(n, vTangent.xyz));   // re-orthogonalise after interpolation
    vec3 b = cross(n, t) * vTangent.w;
    if (!gl_FrontFacing) { n = -n; t = -t; b = -b; }               // back faces of double-sided materials

    vec3 tangentNormal = texture(uNormalTex, vUV).xyz * 2.0 - 1.0;
    tangentNormal.xy *= uNormalScale;
    return normalize(mat3(t, b, n) * tangentNormal);
}

void main()
{
    vec4 baseColor = uBaseColorFactor * texture(uBaseColorTex, vUV);
    vec4 mr = texture(uMetallicRoughnessTex, vUV);
    float metallic = clamp(uMetallicFactor * mr.b, 0.0, 1.0);
    float roughness = clamp(uRoughnessFactor * mr.g, 0.0, 1.0);
    float occlusion = 1.0 + uOcclusionStrength * (texture(uOcclusionTex, vUV).r - 1.0);
    vec3 emissive = uEmissiveFactor * texture(uEmissiveTex, vUV).rgb;
    vec3 n = shadingNormal();

    vec3 color;
    switch (uDebugView) {
    case VIEW_BASE_COLOR: color = baseColor.rgb; break;
    case VIEW_METALLIC:   color = vec3(metallic); break;
    case VIEW_ROUGHNESS:  color = vec3(roughness); break;
    case VIEW_NORMAL:     color = n * 0.5 + 0.5; break;
    case VIEW_OCCLUSION:  color = vec3(occlusion); break;
    case VIEW_EMISSIVE:   color = emissive; break;
    case VIEW_UV:         color = vec3(fract(vUV), 0.0); break;
    default:              color = baseColor.rgb; break;   // Session 4 replaces this with lighting
    }

    fragColor = vec4(color, 1.0);   // linear light; the tone-map pass turns it into display values
}