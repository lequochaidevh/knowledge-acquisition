cmake_minimum_required(VERSION 3.15)
project(ComLinkEngine LANGUAGES CXX)

# Force Modern C++17 Standard Compilation Compliance
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# 1. configuration
set(LibName "rocommlink")
set(INTERNAL_ROOT "$ENV{LOCAL_MINOR_ROOT}")

# Add compilation optimization flags for safety
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -pthread)
endif()

find_package(Threads REQUIRED)

# Core static library targeting the modular architecture components
add_library(${LibName} STATIC
    src/core/common/task_queue.cpp
    src/core/transport/udp_transport.cpp
    src/core/protocol/rocommlink_parser.cpp
    src/core/protocol/packet_serializer.cpp
    src/core/service/rocommlink_dispatcher.cpp
    src/core/service/rocommlink.cpp
)

# Crucial step: Standardize include boundaries for absolute path lookups
target_include_directories(${LibName} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/core>
    $<BUILD_INTERFACE:${INTERNAL_ROOT}/include>
)

# Link platform threads dependency library
target_link_libraries(${LibName} PRIVATE Threads::Threads)

# Enable testing subsystem pipeline execution tree
enable_testing()
add_subdirectory(tests)
