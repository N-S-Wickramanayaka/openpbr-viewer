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
#include "HdrTarget.h"
#include "Environment.h"

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

// Light settings shown in the UI. Positions scale with the model so any glTF gets sensible defaults.
struct Lights {
    float sunAzimuth = glm::radians(45.0f);
    float sunElevation = glm::radians(50.0f);
    glm::vec3 sunColor{1.0f};
    float sunIntensity = 3.0f;
    glm::vec3 pointPosition{2.0f, 1.5f, 2.0f};
    glm::vec3 pointColor{1.0f, 0.85f, 0.7f};
    float pointIntensity = 10.0f;
    float ambient = 0.3f;
};

struct EnvironmentSettings {
    float rotationDegrees = 90.0f;   // 90 puts the panorama's centre at +Z, matching the Khronos Sample Viewer
    float intensity = 1.0f;
    bool showBackground = true;
};

static bool endsWith(const std::string& text, const std::string& suffix)
{
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Loads a model and, only if that worked, replaces the current one and frames the camera on it.
static bool openModel(const std::string& path, Model& model, OrbitCamera& camera, Lights& lights)
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
    lights.pointPosition = center + radius * glm::vec3(1.2f, 1.0f, 1.2f);
    lights.pointIntensity = 3.0f * glm::dot(lights.pointPosition - center, lights.pointPosition - center);
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
    GLuint toneMapProgram = loadProgram(ASSET_DIR "shaders/fullscreen.vert", ASSET_DIR "shaders/tonemap.frag");
    GLuint backgroundProgram = loadProgram(ASSET_DIR "shaders/background.vert", ASSET_DIR "shaders/background.frag");
    if (!pbrProgram || !toneMapProgram || !backgroundProgram) return EXIT_FAILURE;

    GLuint panorama = loadPanorama(ASSET_DIR "environments/Cannon_Exterior.hdr");
    if (!panorama) return EXIT_FAILURE;
    auto openPanorama = [&panorama](const std::string& path) {   // replaces the panorama only if the new one loads
        if (GLuint loaded = loadPanorama(path)) {
            glDeleteTextures(1, &panorama);
            panorama = loaded;
        }
    };
    EnvironmentSettings envSettings;

    GLuint emptyVao = 0;   // core profile needs a VAO bound for every draw, even one with no vertex inputs
    glCreateVertexArrays(1, &emptyVao);

    OrbitCamera camera;
    Lights lights;
    Model model;
    const std::string startModel = argc > 1 ? argv[1] : ASSET_DIR "models/DamagedHelmet.glb";
    if (!openModel(startModel, model, camera, lights)) return EXIT_FAILURE;

    bool vsync = true;
    int debugView = 0;
    HdrTarget hdr;
    int toneMapper = 0;
    float exposureEV = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {   // minimised
            glfwWaitEvents();
            continue;
        }
        if (!g_droppedFile.empty()) {
            if (endsWith(g_droppedFile, ".hdr")) openPanorama(g_droppedFile);
            else openModel(g_droppedFile, model, camera, lights);
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
        ImGui::TextDisabled("Drop a .glb, .gltf or .hdr file on the window to open it");
        if (ImGui::Button("DamagedHelmet")) openModel(ASSET_DIR "models/DamagedHelmet.glb", model, camera, lights);
        ImGui::SameLine();
        if (ImGui::Button("MetalRoughSpheres"))
            openModel(ASSET_DIR "models/MetalRoughSpheres.glb", model, camera, lights);
        ImGui::Combo("View", &debugView, "Lit\0Base colour\0Metallic\0Roughness\0Normal\0Occlusion\0Emissive\0UV\0");
        ImGui::SeparatorText("Camera");
        ImGui::SliderFloat("Exposure (EV)", &exposureEV, -6.0f, 6.0f, "%.1f");
        ImGui::Combo("Tone mapper", &toneMapper, "Khronos PBR Neutral\0ACES (Narkowicz)\0None (clamp)\0");
        ImGui::SeparatorText("Environment");
        if (ImGui::Button("Cannon Exterior")) openPanorama(ASSET_DIR "environments/Cannon_Exterior.hdr");
        ImGui::SameLine();
        if (ImGui::Button("Studio Neutral")) openPanorama(ASSET_DIR "environments/neutral.hdr");
        ImGui::SliderFloat("Rotation", &envSettings.rotationDegrees, -180.0f, 180.0f, "%.0f deg");
        ImGui::SliderFloat("Intensity", &envSettings.intensity, 0.0f, 4.0f);
        ImGui::Checkbox("Background", &envSettings.showBackground);
        ImGui::SeparatorText("Lights");
        ImGui::SliderAngle("Sun azimuth", &lights.sunAzimuth, -180.0f, 180.0f);
        ImGui::SliderAngle("Sun elevation", &lights.sunElevation, -90.0f, 90.0f);
        ImGui::ColorEdit3("Sun colour", &lights.sunColor.x);
        ImGui::DragFloat("Sun intensity", &lights.sunIntensity, 0.05f, 0.0f, 100.0f);
        ImGui::DragFloat3("Point position", &lights.pointPosition.x, 0.05f);
        ImGui::ColorEdit3("Point colour", &lights.pointColor.x);
        ImGui::DragFloat("Point intensity", &lights.pointIntensity, 0.1f, 0.0f, 10000.0f);
        ImGui::DragFloat("Ambient", &lights.ambient, 0.005f, 0.0f, 10.0f);
        ImGui::End();

        // ---- Scene pass: draw the model into the HDR target in linear light ----
        resizeHdrTarget(hdr, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, hdr.fbo);
        glViewport(0, 0, width, height);
        const float clearColor[4] = {0.02f, 0.02f, 0.025f, 1.0f};
        const float clearDepth = 1.0f;
        glClearNamedFramebufferfv(hdr.fbo, GL_COLOR, 0, clearColor);
        glClearNamedFramebufferfv(hdr.fbo, GL_DEPTH, 0, &clearDepth);
        glEnable(GL_DEPTH_TEST);

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Scene");
        glUseProgram(pbrProgram);
        const glm::mat4 viewProj = camera.projection(float(width) / float(height)) * camera.view();
        const glm::mat3 envRotation =
        glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(envSettings.rotationDegrees), glm::vec3(0, 1, 0)));
        glProgramUniformMatrix4fv(pbrProgram, 0, 1, GL_FALSE, glm::value_ptr(viewProj));
        glProgramUniform1i(pbrProgram, 9, debugView);
        const glm::vec3 sunDir(std::cos(lights.sunElevation) * std::sin(lights.sunAzimuth), std::sin(lights.sunElevation),
                               std::cos(lights.sunElevation) * std::cos(lights.sunAzimuth));
        glProgramUniform3fv(pbrProgram, 10, 1, glm::value_ptr(camera.position()));
        glProgramUniform3fv(pbrProgram, 11, 1, glm::value_ptr(sunDir));
        glProgramUniform3fv(pbrProgram, 12, 1, glm::value_ptr(lights.sunColor * lights.sunIntensity));
        glProgramUniform3fv(pbrProgram, 13, 1, glm::value_ptr(lights.pointPosition));
        glProgramUniform3fv(pbrProgram, 14, 1, glm::value_ptr(lights.pointColor * lights.pointIntensity));
        glProgramUniform3fv(pbrProgram, 15, 1, glm::value_ptr(glm::vec3(lights.ambient)));

        for (const Draw& draw : model.draws) {
            const Primitive& prim = model.primitives[draw.primitive];
            const Material& mat = prim.material >= 0 ? model.materials[prim.material] : model.defaultMaterial;
            const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(draw.world)));

            glProgramUniformMatrix4fv(pbrProgram, 1, 1, GL_FALSE, glm::value_ptr(draw.world));
            glProgramUniformMatrix3fv(pbrProgram, 2, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glProgramUniform4fv(pbrProgram, 3, 1, glm::value_ptr(mat.baseColorFactor));
            glProgramUniform1f(pbrProgram, 4, mat.metallicFactor);
            glProgramUniform1f(pbrProgram, 5, mat.roughnessFactor);
            glProgramUniform3fv(pbrProgram, 6, 1, glm::value_ptr(mat.emissiveFactor));
            glProgramUniform1f(pbrProgram, 7, mat.normalScale);
            glProgramUniform1f(pbrProgram, 8, mat.occlusionStrength);

            const GLuint textures[5] = {mat.baseColorTex, mat.metallicRoughnessTex, mat.normalTex, mat.occlusionTex,
                                        mat.emissiveTex};
            glBindTextures(0, 5, textures);   // units 0 to 4, matching the binding = N in pbr.frag

            if (mat.doubleSided) glDisable(GL_CULL_FACE);
            else glEnable(GL_CULL_FACE);
            glBindVertexArray(prim.vao);
            glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        // Background: drawn last, on the far plane, so it only fills pixels the model left empty.
        if (debugView == 0 && envSettings.showBackground) {
            const glm::mat4 viewRotation = glm::mat4(glm::mat3(camera.view()));   // no translation: the sky is infinitely far
            const glm::mat4 invViewRotProj = glm::inverse(camera.projection(float(width) / float(height)) * viewRotation);
            glUseProgram(backgroundProgram);
            glProgramUniformMatrix4fv(backgroundProgram, 0, 1, GL_FALSE, glm::value_ptr(invViewRotProj));
            glProgramUniformMatrix3fv(backgroundProgram, 1, 1, GL_FALSE, glm::value_ptr(envRotation));
            glProgramUniform1f(backgroundProgram, 2, envSettings.intensity);
            glBindTextureUnit(0, panorama);
            glDisable(GL_CULL_FACE);
            glDepthFunc(GL_LEQUAL);    // the far plane has depth 1.0, equal to the cleared depth
            glDepthMask(GL_FALSE);
            glBindVertexArray(emptyVao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }
        glPopDebugGroup();

        // ---- Tone-map pass: HDR target -> screen ----
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Tone map");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glUseProgram(toneMapProgram);
        const bool colourView = debugView == 1 || debugView == 6;   // base colour and emissive are colours
        const int mode = debugView == 0 ? 0 : (colourView ? 1 : 2);
        glProgramUniform1f(toneMapProgram, 0, std::exp2(exposureEV));
        glProgramUniform1i(toneMapProgram, 1, toneMapper);
        glProgramUniform1i(toneMapProgram, 2, mode);
        glBindTextureUnit(0, hdr.color);
        glBindVertexArray(emptyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glPopDebugGroup();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    destroyModel(model);
    glDeleteTextures(1, &panorama);
    glDeleteProgram(backgroundProgram);
    destroyHdrTarget(hdr);
    glDeleteVertexArrays(1, &emptyVao);
    glDeleteProgram(toneMapProgram);
    glDeleteProgram(pbrProgram);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}