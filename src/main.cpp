#include <glad/gl.h>
#define GLFW_INCLUDE_NONE   // glad provides the OpenGL declarations, not GLFW
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include <glm/glm.hpp>
#include <cstddef>
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
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);   // report on the offending call, not later
    glDebugMessageCallback(onGlDebugMessage, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {   // minimised
            glfwWaitEvents();
            continue;
        }

        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(triProgram);
        glBindVertexArray(triVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }

    glDeleteProgram(triProgram);
    glDeleteVertexArrays(1, &triVao);
    glDeleteBuffers(1, &triVbo);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}