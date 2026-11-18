#pragma once
#include <glad/gl.h>
#include <string>

// Loads an 8-bit image as an immutable, mipmapped 2D texture. srgb = true for colour images.
GLuint loadTexture2D(const std::string& path, bool srgb);