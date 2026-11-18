#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION   // in exactly one .cpp file
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

GLuint loadTexture2D(const std::string& path, bool srgb)
{
    stbi_set_flip_vertically_on_load(1);   // OpenGL expects the first row at the bottom
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);   // always RGBA
    if (!pixels) {
        std::fprintf(stderr, "Cannot load %s: %s\n", path.c_str(), stbi_failure_reason());
        return 0;
    }

    const GLsizei levels = 1 + static_cast<GLsizei>(std::floor(std::log2(static_cast<float>(std::max(w, h)))));

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, levels, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, w, h);
    glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateTextureMipmap(tex);

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY, 8.0f);   // core since OpenGL 4.6

    glObjectLabel(GL_TEXTURE, tex, -1, path.c_str());   // shows the file name in RenderDoc
    stbi_image_free(pixels);
    return tex;
}