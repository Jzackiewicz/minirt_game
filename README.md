# MiniRT Game

Puzzle game based on miniRT 42School project.

## How to Play
*To be added*

## Installation

### Windows (MSYS2)
1. Install [MSYS2](https://www.msys2.org/) and open the **MSYS2 UCRT64** shell.
2. Update the system and install dependencies:
   ```bash
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain \
       mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
       mingw-w64-ucrt-x86_64-SDL2
   ```
3. Configure and build the project with CMake:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ```

### Linux (Debian)
1. Install dependencies:
   ```bash
   sudo apt-get update
   sudo apt-get install build-essential cmake libsdl2-dev
   ```
2. Configure and build the project with CMake:
   ```bash
   cmake -S . -B build
   cmake --build build -j
   ```

## Running

### Windows
```bash
./build/minirt.exe scenes/[map].rt
```

### Linux
```bash
./build/minirt scenes/[map].rt
```

The `scenes` directory contains sample `.rt` files.

## Soft shadows

Point lights support soft shadows by giving them a radius. In scene files the
radius can be specified as an optional value after the light color:

```
L 0,5,0 1.0 255,255,255,255 2.0
```

When the radius is greater than zero the renderer casts multiple shadow rays and
blends the result to produce softer penumbra.
