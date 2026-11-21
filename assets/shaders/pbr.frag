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

layout(location = 10) uniform vec3 uCameraPos;
layout(location = 11) uniform vec3 uSunDirection;    // unit vector from the surface towards the sun
layout(location = 12) uniform vec3 uSunIlluminance;  // colour x intensity
layout(location = 13) uniform vec3 uPointPosition;
layout(location = 14) uniform vec3 uPointIntensity;  // colour x intensity, falls off with 1 / distance^2
layout(location = 15) uniform vec3 uAmbient;         // constant fill light; image-based lighting replaces it next week

out vec4 fragColor;

const float PI = 3.14159265359;

// Debug views, in the same order as the ImGui combo
const int VIEW_LIT = 0, VIEW_BASE_COLOR = 1, VIEW_METALLIC = 2, VIEW_ROUGHNESS = 3, VIEW_NORMAL = 4,
          VIEW_OCCLUSION = 5, VIEW_EMISSIVE = 6, VIEW_UV = 7;

// ---------- BRDF terms (glTF 2.0 specification, Appendix B) ----------

// GGX / Trowbridge-Reitz normal distribution: how many microfacets face the half vector
float D_GGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float f = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * f * f);
}

// Height-correlated Smith visibility: shadowing and masking, with the 1 / (4 NdotL NdotV) folded in
float V_SmithGGXCorrelated(float NdotL, float NdotV, float alpha)
{
    float a2 = alpha * alpha;
    float ggxV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    float ggxL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    float sum = ggxV + ggxL;
    return sum > 0.0 ? 0.5 / sum : 0.0;
}

// Schlick's Fresnel approximation: reflectance rises towards 1 at grazing angles
vec3 F_Schlick(vec3 f0, float VdotH)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// Light reflected towards the viewer from one light. radiance = light arriving at the surface.
vec3 shadeLight(vec3 n, vec3 v, vec3 l, vec3 radiance, vec3 baseColor, float metallic, float alpha)
{
    float NdotL = clamp(dot(n, l), 0.0, 1.0);
    if (NdotL <= 0.0) return vec3(0.0);

    vec3 h = l + v;
    float hLength = length(h);
    h = hLength > 1e-5 ? h / hLength : n;   // l == -v would give a zero half vector
    float NdotV = clamp(dot(n, v), 0.0, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    float specular = D_GGX(NdotH, alpha) * V_SmithGGXCorrelated(NdotL, NdotV, alpha);
    vec3 diffuse = baseColor / PI;

    // Dielectrics: diffuse under a clear specular layer with f0 = 0.04 (index of refraction 1.5).
    vec3 dielectric = mix(diffuse, vec3(specular), F_Schlick(vec3(0.04), VdotH));
    // Metals: no diffuse; the specular reflection is tinted by the base colour.
    vec3 metal = F_Schlick(baseColor, VdotH) * specular;

    return mix(dielectric, metal, metallic) * radiance * NdotL;
}

// ---------- Material inputs ----------

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
    default: {
        vec3 v = normalize(uCameraPos - vWorldPos);
        // Roughness 0 would make the GGX peak infinitely sharp for a point light, so clamp it slightly.
        float alpha = max(roughness, 0.045);
        alpha *= alpha;

        color = shadeLight(n, v, uSunDirection, uSunIlluminance, baseColor.rgb, metallic, alpha);

        vec3 toPoint = uPointPosition - vWorldPos;
        float distance2 = max(dot(toPoint, toPoint), 1e-4);
        color += shadeLight(n, v, toPoint * inversesqrt(distance2), uPointIntensity / distance2,
                            baseColor.rgb, metallic, alpha);

        // Crude stand-in for light bouncing in from the surroundings, dimmed by ambient occlusion.
        color += uAmbient * baseColor.rgb * occlusion;
        color += emissive;
    } break;
    }

    fragColor = vec4(color, 1.0);   // linear light; the tone-map pass turns it into display values
}