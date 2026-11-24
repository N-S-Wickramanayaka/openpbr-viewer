#include "Environment.h"
#include "Shader.h"

#include <stb_image.h>   // the implementation is compiled in Texture.cpp

#include <cstdio>

namespace {

int mipCount(int size)
{
    int levels = 1;
    while (size > 1) {
        size /= 2;
        ++levels;
    }
    return levels;
}

GLuint createCubemap(int size, int levels, const char* label)
{
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);
    glTextureStorage2D(tex, levels, GL_RGBA16F, size, size);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glObjectLabel(GL_TEXTURE, tex, -1, label);
    return tex;
}

// Runs the current compute program once per texel of one mip level of a cubemap:
// 8 x 8 texels per work group, and one layer of groups per face.
void dispatchCube(GLuint cubemap, int level, int size)
{
    glBindImageTexture(0, cubemap, level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    const GLuint groups = GLuint((size + 7) / 8);
    glDispatchCompute(groups, groups, 6);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);   // load-time only, so the broadest barrier costs nothing that matters
}

Environment buildFromEquirect(GLuint equirect, int equirectWidth, const IblPrograms& programs)
{
    Environment env;

    // 1. Panorama to cubemap. A face a quarter of the panorama's width keeps all of its detail.
    int size = 256;
    while (size < equirectWidth / 4 && size < 1024) size *= 2;
    env.cubemap = createCubemap(size, mipCount(size), "Environment cubemap");
    glUseProgram(programs.equirectToCube);
    glBindTextureUnit(0, equirect);
    dispatchCube(env.cubemap, 0, size);
    glGenerateTextureMipmap(env.cubemap);   // blurred mips let the filters below use few samples without noise

    return env;
}

GLuint createEquirectTexture(const float* rgba, int width, int height)
{
    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA32F, width, height);   // 32-bit: a bright sun can exceed the 16-bit float range
    glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, rgba);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);         // the panorama wraps around horizontally
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glObjectLabel(GL_TEXTURE, tex, -1, "Environment panorama");
    return tex;
}

}   // namespace

bool loadIblPrograms(IblPrograms& programs)
{
    programs.equirectToCube = loadComputeProgram(ASSET_DIR "shaders/equirect_to_cube.comp");
    return programs.equirectToCube != 0;
}

void destroyIblPrograms(IblPrograms& programs)
{
    glDeleteProgram(programs.equirectToCube);
    programs = IblPrograms{};
}

bool loadEnvironment(const std::string& hdrPath, const IblPrograms& programs, Environment& env)
{
    stbi_set_flip_vertically_on_load(0);   // row 0 is the top of the sky
    int width = 0, height = 0, channels = 0;
    float* pixels = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "Cannot load %s: %s\n", hdrPath.c_str(), stbi_failure_reason());
        return false;
    }
    const GLuint equirect = createEquirectTexture(pixels, width, height);
    stbi_image_free(pixels);

    Environment built = buildFromEquirect(equirect, width, programs);
    glDeleteTextures(1, &equirect);   // only needed to build the cubemap
    destroyEnvironment(env);
    env = built;
    std::printf("Loaded environment %s (%d x %d)\n", hdrPath.c_str(), width, height);
    return true;
}

void destroyEnvironment(Environment& env)
{
    glDeleteTextures(1, &env.cubemap);   // zero names are ignored
    env = Environment{};
}