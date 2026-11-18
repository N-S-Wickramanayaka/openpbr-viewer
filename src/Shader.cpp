#include "Shader.h"

#include <cstdio>
#include <fstream>
#include <sstream>

static std::string readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::fprintf(stderr, "Cannot open %s\n", path.c_str());
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static GLuint compileStage(GLenum stage, const std::string& path)
{
    const std::string source = readFile(path);
    if (source.empty()) return 0;

    GLuint shader = glCreateShader(stage);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Compile error in %s:\n%s\n", path.c_str(), log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint loadProgram(const std::string& vertPath, const std::string& fragPath)
{
    GLuint vs = compileStage(GL_VERTEX_SHADER, vertPath);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragPath);
    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);   // the program keeps what it needs
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Link error (%s + %s):\n%s\n", vertPath.c_str(), fragPath.c_str(), log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}