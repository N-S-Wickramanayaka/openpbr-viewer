#pragma once
#include <glad/gl.h>
#include <cstddef>
#include <string>

// Uploads RGBA8 pixels as an immutable, mipmapped 2D texture. srgb = true for colour data, false for data maps.
GLuint createTexture2D(const unsigned char* rgba, int width, int height, bool srgb, const char* label);

// Decodes a PNG or JPEG held in memory. glTF images are stored top row first and glTF's UV origin is the
// top-left corner, so they are uploaded without a vertical flip.
GLuint loadTexture2DFromMemory(const unsigned char* bytes, std::size_t size, bool srgb, const char* label);

// Loads an image file. flipY = true for images authored for OpenGL's bottom-left UV origin (Phase 0's cube).
GLuint loadTexture2D(const std::string& path, bool srgb, bool flipY);

// A 1x1 texture of one colour, used when a material has no texture in a slot.
GLuint createSolidTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a, const char* label);