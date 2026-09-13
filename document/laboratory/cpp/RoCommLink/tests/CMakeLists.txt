cmake_minimum_required(VERSION 3.15)
project(ComLinkTests VERSION 1.0 LANGUAGES CXX)

# Find the threads library (pthread on Linux)
find_package(Threads REQUIRED)

set(INTERNAL_ROOT "$ENV{LOCAL_MINOR_ROOT}")

# 1. Define your executable and its localized source files
add_executable(sandbox 
    main_test.cpp
)

# 2. Specify the directory for header files using absolute target interfaces
target_include_directories(sandbox PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/functional
    ${INTERNAL_ROOT}/include
)

# 3. Link both the core architecture library and system thread libraries
target_link_libraries(sandbox PRIVATE 
    rocommlink
    Threads::Threads
)
