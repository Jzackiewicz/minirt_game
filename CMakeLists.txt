
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
target_include_directories(minirt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)
    target_link_libraries(minirt PRIVATE SDL2::SDL2main SDL2::SDL2 SDL2_ttf::SDL2_ttf Threads::Threads)
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    target_include_directories(minirt PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(minirt PRIVATE ${SDL2_LIBRARIES} SDL2_ttf::SDL2_ttf Threads::Threads)
endif()
