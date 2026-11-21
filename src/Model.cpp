#include "Model.h"

#define CGLTF_IMPLEMENTATION   // in exactly one .cpp file
#include <cgltf.h>
#include <mikktspace.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <numeric>
#include <utility>

// ---------- Tangents ----------

static Vertex& corner(const SMikkTSpaceContext* ctx, int face, int vert)
{
    return (*static_cast<std::vector<Vertex>*>(ctx->m_pUserData))[std::size_t(face) * 3 + vert];
}

// MikkTSpace is the tangent algorithm the glTF spec asks for when a file has none. It works per triangle
// corner, so we expand the mesh to one vertex per corner, generate, and keep the result unindexed.
static void generateTangents(std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices)
{
    std::vector<Vertex> corners(indices.size());
    for (std::size_t i = 0; i < indices.size(); ++i) corners[i] = vertices[indices[i]];

    SMikkTSpaceInterface callbacks{};
    callbacks.m_getNumFaces = [](const SMikkTSpaceContext* ctx) {
        return int(static_cast<std::vector<Vertex>*>(ctx->m_pUserData)->size() / 3);
    };
    callbacks.m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, int) { return 3; };
    callbacks.m_getPosition = [](const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
        const glm::vec3& p = corner(ctx, face, vert).position;
        out[0] = p.x; out[1] = p.y; out[2] = p.z;
    };
    callbacks.m_getNormal = [](const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
        const glm::vec3& n = corner(ctx, face, vert).normal;
        out[0] = n.x; out[1] = n.y; out[2] = n.z;
    };
    callbacks.m_getTexCoord = [](const SMikkTSpaceContext* ctx, float out[], int face, int vert) {
        const glm::vec2& uv = corner(ctx, face, vert).uv;
        out[0] = uv.x;
        out[1] = 1.0f - uv.y;   // glTF's V axis points down the image; MikkTSpace expects it to point up
    };
    callbacks.m_setTSpaceBasic = [](const SMikkTSpaceContext* ctx, const float t[], float sign, int face, int vert) {
        corner(ctx, face, vert).tangent = glm::vec4(t[0], t[1], t[2], sign);
    };

    SMikkTSpaceContext context{&callbacks, &corners};
    genTangSpaceDefault(&context);

    vertices = std::move(corners);
    std::iota(indices.begin(), indices.end(), 0u);
}

// ---------- Geometry ----------

static bool loadPrimitive(const cgltf_primitive& prim, Primitive& out)
{
    if (prim.type != cgltf_primitive_type_triangles) return false;   // points and lines are skipped

    const cgltf_accessor *pos = nullptr, *nrm = nullptr, *uv = nullptr, *tan = nullptr;
    for (cgltf_size i = 0; i < prim.attributes_count; ++i) {
        const cgltf_attribute& a = prim.attributes[i];
        if (a.type == cgltf_attribute_type_position) pos = a.data;
        else if (a.type == cgltf_attribute_type_normal) nrm = a.data;
        else if (a.type == cgltf_attribute_type_texcoord && a.index == 0) uv = a.data;
        else if (a.type == cgltf_attribute_type_tangent) tan = a.data;
    }
    if (!pos) return false;

    std::vector<Vertex> vertices(pos->count);
    for (cgltf_size v = 0; v < pos->count; ++v) {
        cgltf_accessor_read_float(pos, v, glm::value_ptr(vertices[v].position), 3);
        if (nrm) cgltf_accessor_read_float(nrm, v, glm::value_ptr(vertices[v].normal), 3);
        if (uv) cgltf_accessor_read_float(uv, v, glm::value_ptr(vertices[v].uv), 2);
        if (tan) cgltf_accessor_read_float(tan, v, glm::value_ptr(vertices[v].tangent), 4);
    }

    std::vector<std::uint32_t> indices;
    if (prim.indices) {
        indices.resize(prim.indices->count);
        for (cgltf_size i = 0; i < indices.size(); ++i)
            indices[i] = static_cast<std::uint32_t>(cgltf_accessor_read_index(prim.indices, i));
    } else {
        indices.resize(vertices.size());
        std::iota(indices.begin(), indices.end(), 0u);
    }

    if (!nrm) std::fprintf(stderr, "Warning: a primitive has no normals; lighting on it will be wrong\n");
    if (!tan && nrm && uv) generateTangents(vertices, indices);

    glCreateBuffers(1, &out.vbo);
    glNamedBufferStorage(out.vbo, vertices.size() * sizeof(Vertex), vertices.data(), 0);
    glCreateBuffers(1, &out.ebo);
    glNamedBufferStorage(out.ebo, indices.size() * sizeof(std::uint32_t), indices.data(), 0);

    glCreateVertexArrays(1, &out.vao);
    glVertexArrayVertexBuffer(out.vao, 0, out.vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(out.vao, out.ebo);
    glEnableVertexArrayAttrib(out.vao, 0);
    glVertexArrayAttribFormat(out.vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(out.vao, 0, 0);
    glEnableVertexArrayAttrib(out.vao, 1);
    glVertexArrayAttribFormat(out.vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(out.vao, 1, 0);
    glEnableVertexArrayAttrib(out.vao, 2);
    glVertexArrayAttribFormat(out.vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribBinding(out.vao, 2, 0);
    glEnableVertexArrayAttrib(out.vao, 3);
    glVertexArrayAttribFormat(out.vao, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
    glVertexArrayAttribBinding(out.vao, 3, 0);

    out.indexCount = static_cast<GLsizei>(indices.size());
    return true;
}

// ---------- Model ----------

bool loadModel(const std::string& path, Model& model)
{
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        std::fprintf(stderr, "Cannot parse %s\n", path.c_str());
        return false;
    }
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        std::fprintf(stderr, "Cannot load the buffers of %s\n", path.c_str());
        cgltf_free(data);
        return false;
    }

    model = Model{};
    // Upload every mesh once; remember where each mesh's primitives start.
    std::vector<std::pair<int, int>> meshRanges;   // (first primitive, count) per mesh
    for (cgltf_size m = 0; m < data->meshes_count; ++m) {
        const cgltf_mesh& mesh = data->meshes[m];
        const int first = static_cast<int>(model.primitives.size());
        for (cgltf_size p = 0; p < mesh.primitives_count; ++p) {
            Primitive prim;
            if (!loadPrimitive(mesh.primitives[p], prim)) continue;
            if (mesh.primitives[p].material) prim.material = static_cast<int>(mesh.primitives[p].material - data->materials);
            model.primitives.push_back(prim);
        }
        meshRanges.emplace_back(first, static_cast<int>(model.primitives.size()) - first);
    }

    // Every node that references a mesh draws it with the node's world matrix (parents included).
    glm::vec3 lo(std::numeric_limits<float>::max()), hi(std::numeric_limits<float>::lowest());
    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        if (!node.mesh) continue;
        glm::mat4 world;
        cgltf_node_transform_world(&node, glm::value_ptr(world));

        const auto [first, count] = meshRanges[node.mesh - data->meshes];
        for (int i = 0; i < count; ++i) model.draws.push_back({world, first + i});

        // Bounds: glTF requires min/max on every POSITION accessor, so transform the 8 box corners.
        for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p) {
            for (cgltf_size a = 0; a < node.mesh->primitives[p].attributes_count; ++a) {
                const cgltf_attribute& attr = node.mesh->primitives[p].attributes[a];
                if (attr.type != cgltf_attribute_type_position || !attr.data->has_min || !attr.data->has_max) continue;
                for (int c = 0; c < 8; ++c) {
                    const glm::vec3 boxCorner((c & 1) ? attr.data->max[0] : attr.data->min[0],
                                              (c & 2) ? attr.data->max[1] : attr.data->min[1],
                                              (c & 4) ? attr.data->max[2] : attr.data->min[2]);
                    const glm::vec3 w = glm::vec3(world * glm::vec4(boxCorner, 1.0f));
                    lo = glm::min(lo, w);
                    hi = glm::max(hi, w);
                }
            }
        }
    }
    model.boundsMin = lo;
    model.boundsMax = hi;

    cgltf_free(data);
    std::printf("Loaded %s: %zu primitives, %zu materials, %zu draws\n", path.c_str(), model.primitives.size(),
                model.materials.size(), model.draws.size());
    return !model.draws.empty();
}

void destroyModel(Model& model)
{
    for (Primitive& p : model.primitives) {
        glDeleteVertexArrays(1, &p.vao);
        glDeleteBuffers(1, &p.vbo);
        glDeleteBuffers(1, &p.ebo);
    }
    glDeleteTextures(static_cast<GLsizei>(model.textures.size()), model.textures.data());
    model = Model{};
}