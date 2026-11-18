# OpenPBR Viewer

A real-time OpenPBR renderer in C++ and OpenGL 4.6 that measures its own accuracy against a path-traced reference.

![Phase 0: textured cube with orbit camera](docs/media/phase0.gif)

**Status:** Phase 0 of 8 complete: build system, debug tooling, textured cube.

## Build

Windows: Visual Studio 2026 with "Desktop development with C++". Open the folder in Visual Studio, or in Developer PowerShell run:

    cmake --preset release
    cmake --build --preset release

Arch Linux:

    sudo pacman -S --needed base-devel cmake ninja git wayland libxkbcommon libx11 libxcursor libxi libxinerama libxrandr
    cmake --preset release && cmake --build --preset release

## Licence

MIT
