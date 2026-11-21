#include <glad/gl.h>
#define GLFW_INCLUDE_NONE   // glad provides the OpenGL declarations, not GLFW
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "Camera.h"
#include "Model.h"
#include "Shader.h"

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
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(onGlDebugMessage, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

static std::string g_droppedFile;   // set when a file is dragged onto the window

// Loads a model and, only if that worked, replaces the current one and frames the camera on it.
static bool openModel(const std::string& path, Model& model, OrbitCamera& camera)
{
    Model loaded;
    if (!loadModel(path, loaded)) {
        destroyModel(loaded);   // frees anything created before the failure
        return false;
    }
    destroyModel(model);
    model = loaded;

    const glm::vec3 center = 0.5f * (model.boundsMin + model.boundsMax);
    const float radius = 0.5f * glm::length(model.boundsMax - model.boundsMin);
    camera.frame(center, radius);
    return true;
}

int main(int argc, char** argv)
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
    glfwSwapInterval(1);
    glfwSetDropCallback(window, [](GLFWwindow*, int count, const char** paths) {
        if (count > 0) g_droppedFile = paths[0];
    });

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

    GLuint pbrProgram = loadProgram(ASSET_DIR "shaders/pbr.vert", ASSET_DIR "shaders/pbr.frag");
    if (!pbrProgram) return EXIT_FAILURE;

    OrbitCamera camera;
    Model model;
    const std::string startModel = argc > 1 ? argv[1] : ASSET_DIR "models/DamagedHelmet.glb";
    if (!openModel(startModel, model, camera)) return EXIT_FAILURE;

    bool vsync = true;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {   // minimised
            glfwWaitEvents();
            continue;
        }
        if (!g_droppedFile.empty()) {
            openModel(g_droppedFile, model, camera);
            g_droppedFile.clear();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                camera.yaw -= io.MouseDelta.x * 0.005f;
                camera.pitch += io.MouseDelta.y * 0.005f;
                camera.pitch = glm::clamp(camera.pitch, glm::radians(-89.0f), glm::radians(89.0f));
            }
            camera.radius = glm::clamp(camera.radius * std::pow(0.9f, io.MouseWheel), 0.05f * camera.sceneRadius,
                                       50.0f * camera.sceneRadius);
        }

        ImGui::Begin("Viewer");
        ImGui::Text("%.1f fps (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
        if (ImGui::Checkbox("VSync", &vsync)) glfwSwapInterval(vsync ? 1 : 0);
        ImGui::TextDisabled("Drop a .glb or .gltf file on the window to open it");
        if (ImGui::Button("DamagedHelmet")) openModel(ASSET_DIR "models/DamagedHelmet.glb", model, camera);
        ImGui::SameLine();
        if (ImGui::Button("MetalRoughSpheres")) openModel(ASSET_DIR "models/MetalRoughSpheres.glb", model, camera);
        ImGui::End();

        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Scene");
        glUseProgram(pbrProgram);
        const glm::mat4 viewProj = camera.projection(float(width) / float(height)) * camera.view();
        glProgramUniformMatrix4fv(pbrProgram, 0, 1, GL_FALSE, glm::value_ptr(viewProj));

        for (const Draw& draw : model.draws) {
            const Primitive& prim = model.primitives[draw.primitive];
            const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(draw.world)));
            glProgramUniformMatrix4fv(pbrProgram, 1, 1, GL_FALSE, glm::value_ptr(draw.world));
            glProgramUniformMatrix3fv(pbrProgram, 2, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glBindVertexArray(prim.vao);
            glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glPopDebugGroup();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    destroyModel(model);
    glDeleteProgram(pbrProgram);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}