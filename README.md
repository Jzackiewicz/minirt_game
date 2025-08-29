# MiniRT Game

Puzzle game basend on miniRT 42School project.

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

### Windows (MSYS2)
1. Install [MSYS2](https://www.msys2.org/) and open the **MSYS2 UCRT64** shell.
2. Update the system and install dependencies:
   ```bash
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain \
       mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
       mingw-w64-ucrt-x86_64-SDL2
   ```
3. Configure and build the project:
    ```bash
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
     cmake --build build -j
     ```
     (Use `-G "MinGW Makefiles"` instead of Ninja if you prefer make.)
     The build links against both `SDL2` and `SDL2main` provided by MSYS2.
4. Run the executable:
    ```bash
    ./build/minirt.exe
    ```

## Run

### Windows (MSYS2)
After building, launch the game from the MSYS2 shell:
```bash
./build/minirt.exe scenes/test.rt [width height threads]
```
Example with optional arguments:
```bash
./build/minirt.exe scenes/test.rt 1024 768 4
```

### Linux (Ubuntu)
After building, run the renderer:
```bash
./build/minirt scenes/test.rt [width height threads]
```
Example with optional arguments:
```bash
./build/minirt scenes/test.rt 1024 768 4
```

Optional `width`, `height`, and `threads` arguments control the output image size and number of rendering threads. The `scenes` directory contains sample `.rt` files.

