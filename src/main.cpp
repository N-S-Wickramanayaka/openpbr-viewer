#include <glad/gl.h>
#define GLFW_INCLUDE_NONE   // glad provides the OpenGL declarations, not GLFW
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>   // glm::lookAt, glm::perspective
#include <glm/gtc/type_ptr.hpp>           // glm::value_ptr
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Shader.h"
#include "Texture.h"

// Laptops with two GPUs: ask the driver for the discrete one (Windows only).
#ifdef _WIN32
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void GLAPIENTRY onGlDebugMessage(GLenum, GLenum, GLuint id, GLenum severity, GLsizei,
                                        const GLchar* message, const void*)
{
    const char* level = severity == GL_DEBUG_SEVERITY_HIGH     ? "HIGH"
                        : severity == GL_DEBUG_SEVERITY_MEDIUM ? "MEDIUM"
                        : severity == GL_DEBUG_SEVERITY_LOW    ? "LOW"
                                                               : "INFO";
    std::fprintf(stderr, "[GL %s] (id %u) %s\n", level, id, message);
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        std::fflush(stderr);   // put a breakpoint here; the call stack shows the GL call that failed
    }
}

static void enableGlDebugOutput()
{
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        std::fprintf(stderr, "Warning: no debug context, GL errors will be silent\n");
        return;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);   // report on the offending call, not later
    glDebugMessageCallback(onGlDebugMessage, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// 24 vertices and 36 indices, wound counter-clockwise when seen from outside.
static void makeCube(std::vector<Vertex>& vertices, std::vector<std::uint32_t>& indices)
{
    // For each face: outward normal, the face's "right" axis and its "up" axis (right x up = normal).
    const glm::vec3 faces[6][3] = {
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},  {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
    };
    const glm::vec2 corners[4] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};

    for (const auto& face : faces) {
        const glm::vec3 n = face[0], right = face[1], up = face[2];
        const auto base = static_cast<std::uint32_t>(vertices.size());
        for (const glm::vec2& c : corners) {
            vertices.push_back({0.5f * (n + c.x * right + c.y * up), n, 0.5f * (c + 1.0f)});
        }
        indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
}

struct OrbitCamera {
    glm::vec3 target{0.0f};
    float yaw = glm::radians(35.0f);
    float pitch = glm::radians(25.0f);
    float radius = 3.0f;

    glm::vec3 position() const
    {
        return target + radius * glm::vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch),
                                           std::cos(pitch) * std::cos(yaw));
    }
    glm::mat4 view() const { return glm::lookAt(position(), target, glm::vec3(0.0f, 1.0f, 0.0f)); }
};

int main()
{
    glfwSetErrorCallback([](int code, const char* text) { std::fprintf(stderr, "[GLFW %d] %s\n", code, text); });
    if (!glfwInit()) return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenPBR Viewer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // vsync

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        return EXIT_FAILURE;
    }
    std::printf("OpenGL %s\nRenderer: %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    enableGlDebugOutput();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    const float uiScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    ImGui::GetStyle().ScaleAllSizes(uiScale);
    ImGui::GetStyle().FontScaleDpi = uiScale;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    struct TriVertex { glm::vec3 position; glm::vec3 color; };
    const TriVertex triangle[] = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.2f, 1.0f, 0.2f}},
    {{ 0.0f,  0.5f, 0.0f}, {0.2f, 0.4f, 1.0f}},
    };

    GLuint triVbo = 0;
    glCreateBuffers(1, &triVbo);
    glNamedBufferStorage(triVbo, sizeof(triangle), triangle, 0);

    GLuint triVao = 0;
    glCreateVertexArrays(1, &triVao);
    glVertexArrayVertexBuffer(triVao, 0, triVbo, 0, sizeof(TriVertex));   // binding 0 reads triVbo
    glEnableVertexArrayAttrib(triVao, 0);
    glVertexArrayAttribFormat(triVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(TriVertex, position));
    glVertexArrayAttribBinding(triVao, 0, 0);
    glEnableVertexArrayAttrib(triVao, 1);
    glVertexArrayAttribFormat(triVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(TriVertex, color));
    glVertexArrayAttribBinding(triVao, 1, 0);
    glObjectLabel(GL_BUFFER, triVbo, -1, "Triangle vertices");   // names show up in RenderDoc
    glObjectLabel(GL_VERTEX_ARRAY, triVao, -1, "Triangle VAO");

    GLuint triProgram = loadProgram(ASSET_DIR "shaders/triangle.vert", ASSET_DIR "shaders/triangle.frag");
    if (!triProgram) return EXIT_FAILURE;

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    makeCube(vertices, indices);

    GLuint vbo = 0, ebo = 0, vao = 0;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, vertices.size() * sizeof(Vertex), vertices.data(), 0);
    glCreateBuffers(1, &ebo);
    glNamedBufferStorage(ebo, indices.size() * sizeof(std::uint32_t), indices.data(), 0);

    glCreateVertexArrays(1, &vao);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, ebo);
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(vao, 1, 0);
    glEnableVertexArrayAttrib(vao, 2);
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    glVertexArrayAttribBinding(vao, 2, 0);
    glObjectLabel(GL_BUFFER, vbo, -1, "Cube vertices");
    glObjectLabel(GL_BUFFER, ebo, -1, "Cube indices");
    glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "Cube VAO");

    GLuint cubeProgram = loadProgram(ASSET_DIR "shaders/cube.vert", ASSET_DIR "shaders/cube.frag");
    GLuint albedo = loadTexture2D(ASSET_DIR "textures/checker.png", true);
    if (!cubeProgram || !albedo) return EXIT_FAILURE;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);   // skips the back faces, which the winding above makes reliable

    OrbitCamera camera;
    glm::vec3 background{0.08f, 0.08f, 0.10f};
    bool showTriangle = false;
    bool vsync = true;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {   // minimised
            glfwWaitEvents();
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {   // the UI gets the mouse first
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                camera.yaw -= io.MouseDelta.x * 0.005f;
                camera.pitch += io.MouseDelta.y * 0.005f;
                camera.pitch = glm::clamp(camera.pitch, glm::radians(-89.0f), glm::radians(89.0f));
            }
            camera.radius = glm::clamp(camera.radius * std::pow(0.9f, io.MouseWheel), 0.5f, 50.0f);
        }

        ImGui::Begin("Viewer");
        ImGui::Text("%.1f fps (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
        ImGui::ColorEdit3("Background", &background.x);
        ImGui::Checkbox("Show triangle", &showTriangle);
        if (ImGui::Checkbox("VSync", &vsync)) glfwSwapInterval(vsync ? 1 : 0);
        if (ImGui::Button("Reset camera")) camera = OrbitCamera{};
        ImGui::End();

        glViewport(0, 0, width, height);
        glClearColor(background.r, background.g, background.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (showTriangle) {
            glUseProgram(triProgram);
            glBindVertexArray(triVao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        } else {
            const glm::mat4 proj = glm::perspective(glm::radians(45.0f), float(width) / float(height), 0.1f, 100.0f);
            const glm::mat4 mvp = proj * camera.view();   // the model matrix is identity for now

            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Cube");   // a named group in RenderDoc
            glUseProgram(cubeProgram);
            glProgramUniformMatrix4fv(cubeProgram, 0, 1, GL_FALSE, glm::value_ptr(mvp));
            glBindTextureUnit(0, albedo);
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, GLsizei(indices.size()), GL_UNSIGNED_INT, nullptr);
            glPopDebugGroup();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteTextures(1, &albedo);
    glDeleteProgram(cubeProgram);
    glDeleteVertexArrays(1, &vao);
    GLuint cubeBuffers[] = {vbo, ebo};
    glDeleteBuffers(2, cubeBuffers);
    glDeleteProgram(triProgram);
    glDeleteVertexArrays(1, &triVao);
    glDeleteBuffers(1, &triVbo);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}   