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
   (Use `-G "MinGW Makefiles"` instead of Ninja if you prefer make.)

### Linux (Debian)
1. Install dependencies:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake libsdl2-dev
   ```
2. Configure and build the project with CMake:
   ```bash
   cmake -S . -B build
   cmake --build build -j
   ```

## Running

### Windows
```bash
./build/minirt.exe scenes/test.rt [width height threads]
```

### Linux
```bash
./build/minirt scenes/test.rt [width height threads]
```

Optional `width`, `height`, and `threads` arguments control the output image size and number of rendering threads. The `scenes` directory contains sample `.rt` files.
