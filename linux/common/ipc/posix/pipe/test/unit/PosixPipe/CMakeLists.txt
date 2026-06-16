cmake_minimum_required(VERSION 3.10)

project(PosixPipeProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Read minor root
# Check if the external environment variable exists
if(DEFINED ENV{LOCAL_MINOR_ROOT})
    # Read the environment variable and force it into CMAKE_INSTALL_PREFIX
    set(INTERNAL_ROOT_SEARCH_PATH "$ENV{LOCAL_MINOR_ROOT}")
    set(CMAKE_INSTALL_PREFIX "$ENV{LOCAL_MINOR_ROOT}" CACHE PATH "Install path from env" FORCE)
    message(STATUS "--> Found INTERNAL_ROOT_SEARCH_PATH to: ${INTERNAL_ROOT_SEARCH_PATH}")
    message(STATUS "--> Found environment path. Installing to: ${CMAKE_INSTALL_PREFIX}")
else()
    message(FATAL_ERROR "--> Environment variable not found. Using default: ${CMAKE_INSTALL_PREFIX}")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")    
    endif()
endif()

# 2. Tell CMake where to look for custom .cmake module files
if(DEFINED ENV{CMAKE_UTIL_PATH})
    set(CMAKE_UTIL_PATH "$ENV{CMAKE_UTIL_PATH}")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_UTIL_PATH}")
    message(STATUS "--> Found CMAKE_UTIL_PATH to: ${CMAKE_UTIL_PATH}")
else()
    message(FATAL_ERROR "Bootstrap failed: Could not locate the 'cmake' module directory!")
endif()

# 3. Include your custom module file (Looks for FindParentDirectory.cmake in CMAKE_MODULE_PATH)
include(FindParentDirectory)

# 4. Now use your helper function freely!
find_parent_directory("common" TEST_ROOT_DIR)
message(STATUS "TEST_ROOT_DIR is verified at: ${TEST_ROOT_DIR}")

file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/sandbox.cpp"
    "${TEST_ROOT_DIR}/ipc/posix/pipe/*.cpp"
)

message(STATUS "--> Searching for logging library in: ${INTERNAL_ROOT_SEARCH_PATH}")
# 2. Find Header and logging static library in the path
find_path(LOGGING_INCLUDE_DIR 
    NAMES logger.h
    PATHS ${INTERNAL_ROOT_SEARCH_PATH}/include/logging
    NO_DEFAULT_PATH
)

find_library(LOGGING_LIBRARY 
    NAMES logging
    PATHS ${INTERNAL_ROOT_SEARCH_PATH}/lib
    NO_DEFAULT_PATH
)

find_package(fmt REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)


# Enable CMake's testing infrastructure
enable_testing()
# Find the installed GoogleTest package
find_package(GTest REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)


add_executable(sandbox ${SOURCES})

find_package(Threads REQUIRED)

message(STATUS "--> Searching for LOGGING_LIBRARY in: ${LOGGING_LIBRARY}")
message(STATUS "--> Searching for TEST_ROOT_DIR in: ${TEST_ROOT_DIR}")

target_include_directories(sandbox PRIVATE 
    "${TEST_ROOT_DIR}/ipc/posix/pipe/"
    "${TEST_ROOT_DIR}/ipc/"
    "${TEST_ROOT_DIR}"
    ${LOGGING_INCLUDE_DIR}
)

# 3. Tell CMake to pull in GTest headers and binary symbols
# This automatically maps the '::testing' namespace to your compiler paths
target_link_libraries(sandbox PRIVATE 
    Threads::Threads
    ${LOGGING_LIBRARY} 
    fmt::fmt
    GTest::gtest_main
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 4. Enable test discovery (optional but recommended)
include(GoogleTest)
gtest_discover_tests(sandbox)
