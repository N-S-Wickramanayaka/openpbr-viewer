#include "HdrTarget.h"

#include <cstdio>

void destroyHdrTarget(HdrTarget& target)
{
    glDeleteFramebuffers(1, &target.fbo);   // deleting 0 is a no-op, so this is safe on a fresh target
    glDeleteTextures(1, &target.color);
    glDeleteTextures(1, &target.depth);
    target = HdrTarget{};
}

void resizeHdrTarget(HdrTarget& target, int width, int height)
{
    if (target.fbo && target.width == width && target.height == height) return;
    destroyHdrTarget(target);
    target.width = width;
    target.height = height;

    glCreateTextures(GL_TEXTURE_2D, 1, &target.color);
    glTextureStorage2D(target.color, 1, GL_RGBA16F, width, height);
    glTextureParameteri(target.color, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(target.color, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(target.color, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(target.color, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glObjectLabel(GL_TEXTURE, target.color, -1, "HDR colour");

    glCreateTextures(GL_TEXTURE_2D, 1, &target.depth);
    glTextureStorage2D(target.depth, 1, GL_DEPTH_COMPONENT32F, width, height);
    glObjectLabel(GL_TEXTURE, target.depth, -1, "HDR depth");

    glCreateFramebuffers(1, &target.fbo);
    glNamedFramebufferTexture(target.fbo, GL_COLOR_ATTACHMENT0, target.color, 0);
    glNamedFramebufferTexture(target.fbo, GL_DEPTH_ATTACHMENT, target.depth, 0);
    glObjectLabel(GL_FRAMEBUFFER, target.fbo, -1, "HDR target");

    if (glCheckNamedFramebufferStatus(target.fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "HDR framebuffer is incomplete\n");
    }
}