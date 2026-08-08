cmake_minimum_required(VERSION 3.20)
project(RetroNode VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch SDL3 dependency
include(FetchContent)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL3 tests" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build SDL3 as shared library" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3)

# Engine sources
set(ENGINE_SOURCES
    src/platform/main.cpp
    src/core/math/fixed16.h
)

add_executable(retronode ${ENGINE_SOURCES})

target_include_directories(retronode PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (TARGET SDL3::SDL3-shared)
    target_link_libraries(retronode PRIVATE SDL3::SDL3-shared)
elseif (TARGET SDL3::SDL3)
    target_link_libraries(retronode PRIVATE SDL3::SDL3)
else()
    target_link_libraries(retronode PRIVATE SDL3)
endif()

if (WIN32)
    # Enable dynamic module handling symbols export/import
    target_compile_definitions(retronode PRIVATE RN_PLATFORM_WINDOWS)
else()
    target_link_libraries(retronode PRIVATE ${CMAKE_DL_LIBS})
endif()

# Build the game library alongside the engine
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MyRPG/CMakeLists.txt")
    add_subdirectory(MyRPG)
endif()
