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

// Reads a shader and pastes in every line of the form  #include "file.glsl"  (path relative to the shader).
// GLSL has no #include of its own. The #line directives keep compiler errors pointing at the right line
// of the including file.
static std::string loadShaderSource(const std::string& path, int depth = 0)
{
    const std::string text = readFile(path);
    if (text.empty() || depth > 4) return text;   // the depth limit stops a file that includes itself
    const std::string folder = path.substr(0, path.find_last_of("/\\") + 1);

    std::istringstream lines(text);
    std::string line, out;
    int lineNumber = 0;
    while (std::getline(lines, line)) {
        ++lineNumber;
        if (line.rfind("#include \"", 0) == 0) {
            const std::size_t start = line.find('"') + 1;
            const std::size_t end = line.find('"', start);
            const std::string included = loadShaderSource(folder + line.substr(start, end - start), depth + 1);
            if (included.empty()) return {};
            out += "#line 1\n" + included + "\n#line " + std::to_string(lineNumber + 1) + "\n";
        } else {
            out += line + "\n";
        }
    }
    return out;
}

static GLuint compileStage(GLenum stage, const std::string& path)
{
    const std::string source = loadShaderSource(path);
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

static GLuint linkProgram(const GLuint* shaders, int count, const std::string& name)
{
    GLuint program = glCreateProgram();
    for (int i = 0; i < count; ++i) glAttachShader(program, shaders[i]);
    glLinkProgram(program);
    for (int i = 0; i < count; ++i) glDeleteShader(shaders[i]);   // the program keeps what it needs

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Link error (%s):\n%s\n", name.c_str(), log);
        glDeleteProgram(program);
        return 0;
    }
    glObjectLabel(GL_PROGRAM, program, -1, name.c_str());
    return program;
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
    const GLuint shaders[2] = {vs, fs};
    return linkProgram(shaders, 2, fragPath);
}

GLuint loadComputeProgram(const std::string& path)
{
    GLuint cs = compileStage(GL_COMPUTE_SHADER, path);
    if (!cs) return 0;
    return linkProgram(&cs, 1, path);
}