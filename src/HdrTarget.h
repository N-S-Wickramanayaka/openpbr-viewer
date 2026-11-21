#pragma once
#include <glad/gl.h>

// An offscreen framebuffer with a 16-bit float colour buffer and a depth buffer.
// The scene renders into it in linear light; the tone-map pass reads it.
struct HdrTarget {
    GLuint fbo = 0;
    GLuint color = 0;
    GLuint depth = 0;
    int width = 0;
    int height = 0;
};

// Creates the target, or recreates it when the size changed. Cheap to call every frame.
void resizeHdrTarget(HdrTarget& target, int width, int height);
void destroyHdrTarget(HdrTarget& target);