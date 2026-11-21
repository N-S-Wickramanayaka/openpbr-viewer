#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION   // in exactly one .cpp file
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

GLuint createTexture2D(const unsigned char* rgba, int width, int height, bool srgb, const char* label)
{
    const GLsizei levels = 1 + static_cast<GLsizei>(std::floor(std::log2(static_cast<float>(std::max(width, height)))));

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, levels, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, width, height);
    glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateTextureMipmap(tex);

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY, 8.0f);   // core since OpenGL 4.6

    glObjectLabel(GL_TEXTURE, tex, -1, label);
    return tex;
}

GLuint loadTexture2DFromMemory(const unsigned char* bytes, std::size_t size, bool srgb, const char* label)
{
    stbi_set_flip_vertically_on_load(0);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(bytes, static_cast<int>(size), &w, &h, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Cannot decode %s: %s\n", label, stbi_failure_reason());
        return 0;
    }
    GLuint tex = createTexture2D(pixels, w, h, srgb, label);
    stbi_image_free(pixels);
    return tex;
}

GLuint loadTexture2D(const std::string& path, bool srgb, bool flipY)
{
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Cannot load %s: %s\n", path.c_str(), stbi_failure_reason());
        return 0;
    }
    GLuint tex = createTexture2D(pixels, w, h, srgb, path.c_str());
    stbi_image_free(pixels);
    return tex;
}

GLuint createSolidTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a, const char* label)
{
    const unsigned char pixel[4] = {r, g, b, a};
    return createTexture2D(pixel, 1, 1, false, label);
}