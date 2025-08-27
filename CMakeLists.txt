
cmake_minimum_required(VERSION 3.16)
project(minirt_cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Speed flags (user can tune)
if (MSVC)
    add_compile_options(/O2 /fp:fast /DNOMINMAX)
else()
    add_compile_options(-O3 -ffast-math -fno-exceptions -fno-rtti -march=native)
endif()

add_executable(minirt
    src/main.cpp
    src/Parser.cpp
    src/Renderer.cpp
)

find_package(Threads REQUIRED)
find_package(SDL2 QUIET CONFIG)
if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found, fetching with FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.3
    )
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)
endif()
target_include_directories(minirt PRIVATE include)
target_link_libraries(minirt PRIVATE Threads::Threads SDL2::SDL2)
