# MiniRT C++ Skeleton

A minimal ray tracer skeleton implemented in C++.

## Prerequisites
- CMake >= 3.16
- C++17 compatible compiler (e.g., GCC, Clang, MSVC)
- SDL2 development libraries (CMake fetches SDL2 automatically if missing)
- Make or another build tool

### Installing SDL2
Install the SDL2 development packages using your system package manager:

- Ubuntu/Debian: `sudo apt install libsdl2-dev`

## Build
```bash
cmake -S . -B build
cmake --build build
```

## Run
After building, run the renderer with a scene file:
```bash
./build/minirt scenes/test.rt [width height threads]
```
Optional `width`, `height`, and `threads` arguments control the output image size and number of rendering threads. The `scenes` directory contains sample `.rt` files.

