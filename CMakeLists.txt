cmake_minimum_required(VERSION 3.20)

project(meshVisual VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sensible default build type for single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_executable(meshVisual
    src/main.cpp
    src/App.cpp
    src/core/Window.cpp
    src/core/Renderer.cpp
    src/graph/Node.cpp
    src/graph/Edge.cpp
    src/graph/Graph.cpp
    src/graph/GraphGenerator.cpp
)

target_include_directories(meshVisual
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# Basic warnings
if(MSVC)
    target_compile_options(meshVisual PRIVATE /W4)
else()
    target_compile_options(meshVisual PRIVATE -Wall -Wextra -Wpedantic)
endif()