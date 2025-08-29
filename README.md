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
./build/minirt.exe scenes/[map].rt [width height L|M|H]
```

### Linux
```bash
./build/minirt scenes/[map].rt [width height L|M|H]
```

Optional `width`, `height`, and a final quality argument control the output image size and internal render scale. The renderer automatically uses all available hardware threads. Quality can be specified with `L`, `M`, or `H` (low, medium, high) and defaults to `H`.

For example:

```bash
./build/minirt scenes/[map].rt 1080 720 L
```

You can omit the resolution while still supplying quality at the end, or specify resolution and quality together:

```bash
./build/minirt scenes/[map].rt L
./build/minirt scenes/[map].rt 1080 720 L
```

`M` renders at two-thirds resolution (dividing width and height by 1.5) and `L` renders at half resolution (dividing by 2) while scaling the result to the requested window size.
The `scenes` directory contains sample `.rt` files.
