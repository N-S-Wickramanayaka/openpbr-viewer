#pragma once
#include <glad/gl.h>
#include <string>

// Compiles and links a vertex + fragment shader pair. Returns 0 and prints the log on failure.
// Both files may use  #include "other.glsl"  to paste in shared code.
GLuint loadProgram(const std::string& vertPath, const std::string& fragPath);

// Compiles and links a compute shader. Returns 0 and prints the log on failure.
GLuint loadComputeProgram(const std::string& path);