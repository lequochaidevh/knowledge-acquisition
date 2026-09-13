cmake_minimum_required(VERSION 3.15)
project(GpuMathLibrary LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")
cmake_minimum_required(VERSION 3.10)
project(YourProjectName)

# TODO UPDATE PC
# 1. FORCE CMake's internal cache to lock onto your local paths permanently
# This blocks GLFW's internal find_package(OpenGL) from looking at the host system root!
set(OPENGL_gl_LIBRARY "$ENV{LOCAL_MINOR_ROOT}/usr/lib/x86_64-linux-gnu/libOpenGL.so" CACHE FILEPATH "" FORCE)
set(OPENGL_opengl_LIBRARY "$ENV{LOCAL_MINOR_ROOT}/usr/lib/x86_64-linux-gnu/libOpenGL.so" CACHE FILEPATH "" FORCE)
set(OPENGL_INCLUDE_DIR "$ENV{LOCAL_MINOR_ROOT}/usr/include" CACHE PATH "" FORCE)

# 2. Tell CMake to prefer your local path for packages
set(CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}" ${CMAKE_PREFIX_PATH})

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_library(gpu_math STATIC src/gpu_math.cpp)

target_include_directories(gpu_math PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${OPENGL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>    
)

# Link your local variable here
target_link_libraries(gpu_math PUBLIC glad::glad glfw ${OPENGL_gl_LIBRARY})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gpu_math PRIVATE -O3 -march=native)
endif()

install(TARGETS gpu_math DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib_test)
install(FILES include/gpu_math.h DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib_test/include)


# Include secondary subdirectories for isolated pipeline testing modules
add_subdirectory(benchmark)