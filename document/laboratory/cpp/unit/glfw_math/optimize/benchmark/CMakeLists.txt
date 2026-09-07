cmake_minimum_required(VERSION 3.15)
project(LibraryTest LANGUAGES CXX)

set(CMAKE_CXX_STREAM_FLAGS "-O3 -march=native")

# Manually point system to your local compilation include folder
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib_test/include)

# Tell CMake to query package dependencies needed underneath your static engine
list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(run_bench main.cpp)

# Link explicitly with your static asset binary along with system libraries
target_link_libraries(run_bench PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib_test/libgpu_math.a
    glad::glad
    glfw
    OpenGL::GL
)