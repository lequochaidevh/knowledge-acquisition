cmake_minimum_required(VERSION 3.16)
project(VIOCameraApp VERSION 1.0)

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

# ===== Threads =====
find_package(Threads REQUIRED)

# ===== OpenCV =====
find_package(OpenCV REQUIRED)

# ===== nlohmann json =====
find_package(nlohmann_json REQUIRED)

# ===== Include dirs =====
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# ===== Source files =====
file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CamImpl/OpenCV/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sample/*.cpp
)

# ===== Executable =====
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Camera
        ${CMAKE_CURRENT_SOURCE_DIR}/Config
        ${CMAKE_CURRENT_SOURCE_DIR}/CamImpl/OpenCV
)

# ===== Link libraries =====
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# ===== Warnings (good practice) =====
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic
)

# ===== Copy config file after build =====
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/Asset
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/Asset
)

# Use server split camera stream for client (VIO, Detection, Tracking, ...)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Camera-Server)
