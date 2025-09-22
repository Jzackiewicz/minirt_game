# MiniRT Game

Puzzle game based on miniRT 42School project.

## How to Play
*To be added*

## Installation

### Windows (MSYS2)
**Note:** Do not use WSL2 for this setup, as it hinders mouse usage and makes movement control very difficult.

1. Install [MSYS2](https://www.msys2.org/) and open the **MSYS2 UCRT64** shell.
2. Update the system and install dependencies (the MSYS2 terminal might need to restart during installation, so you may need to paste and run these commands again):
   ```bash
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain \
       mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
       mingw-w64-ucrt-x86_64-SDL2 git
   ```
3. Clone this repository:
   ```bash
   git clone https://github.com/Jzackiewicz/minirt_game.git
   ```
4. Configure and build the project with CMake:
   ```bash
   cd minirt_game
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
./build/minirt.exe
```

### Linux
```bash
./build/minirt
```
