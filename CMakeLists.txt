cmake_minimum_required(VERSION 3.25)
project(OpenPBRViewer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # lets clangd and other tools understand the code

include(cmake/Dependencies.cmake)

add_executable(viewer
    src/main.cpp
    src/Shader.cpp
    src/Texture.cpp
)
target_include_directories(viewer PRIVATE src)
target_link_libraries(viewer PRIVATE glad glfw glm::glm imgui stb)

# Week 1 shortcut: the exe finds shaders and textures through an absolute path baked in at configure time.
target_compile_definitions(viewer PRIVATE ASSET_DIR="${PROJECT_SOURCE_DIR}/assets/")

if(MSVC)
    target_compile_options(viewer PRIVATE /W4 /permissive-)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT viewer)
else()
    target_compile_options(viewer PRIVATE -Wall -Wextra -Wpedantic)
endif()
