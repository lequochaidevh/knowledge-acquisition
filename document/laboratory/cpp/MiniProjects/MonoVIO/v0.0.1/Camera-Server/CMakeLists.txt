cmake_minimum_required(VERSION 3.16)
project(CameraServer VERSION 1.0)

# ===== C++ Standard =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ===== Build Type =====
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ===== Compiler Flags (Optimize CPU) =====
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")


find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

# ===== Include dirs =====
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# ===== Source files =====
file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# ===== Executable =====
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${GSTREAMER_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${GSTREAMER_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES})

target_compile_options(${PROJECT_NAME} PRIVATE
    ${GSTREAMER_CFLAGS_OTHER}
    -Wall -Wextra -Wpedantic
)
