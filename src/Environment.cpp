#include "Environment.h"

#include <stb_image.h>   // the implementation is compiled in Texture.cpp

#include <cstdio>

GLuint loadPanorama(const std::string& hdrPath)
{
    stbi_set_flip_vertically_on_load(0);   // row 0 is the top of the sky
    int width = 0, height = 0, channels = 0;
    float* pixels = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Cannot load %s: %s\n", hdrPath.c_str(), stbi_failure_reason());
        return 0;
    }

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA32F, width, height);   // 32-bit: a bright sun can exceed the 16-bit float range
    glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, pixels);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);         // the panorama wraps around horizontally
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glObjectLabel(GL_TEXTURE, tex, -1, "Environment panorama");

    stbi_image_free(pixels);
    std::printf("Loaded environment %s (%d x %d)\n", hdrPath.c_str(), width, height);
    return tex;
}