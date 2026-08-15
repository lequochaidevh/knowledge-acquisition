cmake_minimum_required(VERSION 3.14)
project(ftxui_test LANGUAGES CXX)

# Enforce C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Instruct CMake to search inside your custom installation path
list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")

# Locate the installed FTXUI package
find_package(ftxui REQUIRED)

add_executable(ftxui_test 01_split_window.cpp)

# Link against the single consolidated target
target_link_libraries(ftxui_test PRIVATE ftxui::ftxui)
