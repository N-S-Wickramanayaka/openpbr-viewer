#version 460 core

layout(binding = 0) uniform sampler2D uHdr;

layout(location = 0) uniform float uExposure;   // linear multiplier: 2^EV
layout(location = 1) uniform int uToneMapper;   // 0 Khronos PBR Neutral, 1 ACES (Narkowicz fit), 2 clamp only
layout(location = 2) uniform int uMode;         // 0 tone map + sRGB, 1 sRGB only, 2 raw values

out vec4 fragColor;

// Khronos PBR Neutral: keeps base colours accurate up to 0.76, then rolls highlights off towards white.
// Reference implementation from the Khronos PBR Neutral specification.
vec3 toneMapPbrNeutral(vec3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, newPeak * vec3(1.0), g);
}

// Krzysztof Narkowicz's fit of the ACES filmic curve: more contrast, shifts saturated colours.
vec3 toneMapAcesNarkowicz(vec3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Exact sRGB encoding (the piecewise curve, not the pow(1/2.2) approximation).
vec3 linearToSrgb(vec3 c)
{
    c = clamp(c, 0.0, 1.0);
    return mix(12.92 * c, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

void main()
{
    vec3 color = texelFetch(uHdr, ivec2(gl_FragCoord.xy), 0).rgb;   // same size as the screen: one texel per pixel

    if (uMode == 0) {
        color *= uExposure;
        if (uToneMapper == 0) color = toneMapPbrNeutral(color);
        else if (uToneMapper == 1) color = toneMapAcesNarkowicz(color);
    }
    if (uMode != 2) color = linearToSrgb(color);

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}