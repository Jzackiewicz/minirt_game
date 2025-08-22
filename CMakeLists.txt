
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
target_include_directories(minirt PRIVATE include)
target_link_libraries(minirt PRIVATE Threads::Threads)