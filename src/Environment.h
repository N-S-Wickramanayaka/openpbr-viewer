#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>

// Compute programs that turn an HDR panorama into the maps image-based lighting needs. Loaded once.
struct IblPrograms {
    GLuint equirectToCube = 0;
};

struct Environment {
    GLuint cubemap = 0;       // the environment itself, with mips: drawn as the background, read by the filters
};

bool loadIblPrograms(IblPrograms& programs);
void destroyIblPrograms(IblPrograms& programs);

// Loads an equirectangular .hdr panorama and builds the cubemaps. Leaves env unchanged on failure.
bool loadEnvironment(const std::string& hdrPath, const IblPrograms& programs, Environment& env);

void destroyEnvironment(Environment& env);