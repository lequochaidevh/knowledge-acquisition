cmake_minimum_required(VERSION 3.15)
project(GpuMathLibrary LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# 📦 Define Static Library target output 
add_library(gpu_math STATIC src/gpu_math.cpp)

target_include_directories(gpu_math PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gpu_math PUBLIC glad::glad glfw OpenGL::GL)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gpu_math PRIVATE -O3 -march=native)
endif()

install(TARGETS gpu_math DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib_test)
install(FILES include/gpu_math.h DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib_test/include)


# Include secondary subdirectories for isolated pipeline testing modules
add_subdirectory(benchmark)