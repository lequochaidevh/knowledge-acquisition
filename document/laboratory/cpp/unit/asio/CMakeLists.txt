cmake_minimum_required(VERSION 3.14)
project(ftxui_test LANGUAGES CXX)

# Enforce C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Instruct CMake to search inside your custom installation path
list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")

find_package(Threads REQUIRED)

# Define the standalone macro for all targets globally
add_compile_definitions(ASIO_STANDALONE)

add_executable(sandbox 02_benmark.cpp)

target_include_directories(sandbox PRIVATE $ENV{LOCAL_MINOR_ROOT}/include)
# Link with the system threads target
target_link_libraries(sandbox PRIVATE Threads::Threads)
