#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};   // xyz = tangent, w = bitangent sign
};

// glTF metal/rough material. Every texture slot always holds a valid texture (a 1x1 default when the
// file has none), so the shader never needs to branch on "has texture".
struct Material {
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec3 emissiveFactor{0.0f};
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    bool doubleSided = false;
    GLuint baseColorTex = 0;
    GLuint metallicRoughnessTex = 0;
    GLuint normalTex = 0;
    GLuint occlusionTex = 0;
    GLuint emissiveTex = 0;
};

struct Primitive {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    int material = -1;   // index into Model::materials, -1 = the glTF default material
};

struct Draw {   // one primitive placed in the world by a node
    glm::mat4 world{1.0f};
    int primitive = 0;
};

struct Model {
    std::vector<Primitive> primitives;
    std::vector<Material> materials;
    Material defaultMaterial;
    std::vector<Draw> draws;
    std::vector<GLuint> textures;   // every texture the model owns, for cleanup
    glm::vec3 boundsMin{0.0f}, boundsMax{0.0f};
};

bool loadModel(const std::string& path, Model& model);   // .gltf or .glb
void destroyModel(Model& model);
