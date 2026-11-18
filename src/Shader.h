#pragma once
#include <glad/gl.h>
#include <string>

// Compiles and links a vertex + fragment shader pair. Returns 0 and prints the log on failure.
GLuint loadProgram(const std::string& vertPath, const std::string& fragPath);