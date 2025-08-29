
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

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(minirt ${SRC_FILES})

find_package(Threads REQUIRED)

# Try to locate an existing SDL2 installation.  If not found, fetch and build
# SDL2 from source so that users without system packages can still build the
# project, as advertised in the README.
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    include(FetchContent)
    FetchContent_Declare(SDL2
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.2/SDL2-2.30.2.zip
    )
    FetchContent_MakeAvailable(SDL2)
endif()

target_include_directories(minirt PRIVATE include)
target_link_libraries(minirt PRIVATE Threads::Threads SDL2::SDL2)
