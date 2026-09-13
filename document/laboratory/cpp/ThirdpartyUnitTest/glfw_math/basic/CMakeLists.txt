cmake_minimum_required(VERSION 3.15)
project(GPGPU_MatrixMultiply LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(mat_mul benchmark.cpp)

target_compile_features(mat_mul PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mat_mul PRIVATE -O3 -march=native)
endif()

target_link_libraries(mat_mul PRIVATE 
    glad::glad 
    glfw 
    OpenGL::GL 
    glm::glm
)

configure_file(matrix_mul.comp matrix_mul.comp COPYONLY)