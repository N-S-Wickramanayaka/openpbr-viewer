#version 460 core

in vec2 vUV;

layout(binding = 0) uniform sampler2D uAlbedo;

out vec4 fragColor;

void main()
{
    vec3 linearColor = texture(uAlbedo, vUV).rgb;               // linear, because the texture is sRGB
    fragColor = vec4(pow(linearColor, vec3(1.0 / 2.2)), 1.0);   // back to display encoding
}