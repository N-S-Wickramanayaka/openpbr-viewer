#pragma once
#include <glad/gl.h>
#include <string>

// Loads an equirectangular .hdr panorama into a floating-point 2D texture. Returns 0 on failure.
// Session 2 replaces this file with cubemap-based environments.
GLuint loadPanorama(const std::string& hdrPath);