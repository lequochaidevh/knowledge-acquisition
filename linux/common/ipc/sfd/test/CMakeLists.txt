cmake_minimum_required(VERSION 3.10)
project(PosixPipeProject LANGUAGES CXX)

# 1. ONE-LINE BOOTSTRAP: Load the complete global ecosystem directly from your environment variable path
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

# Load the modern automated framework builder
include(GTestSetup)

# 2. Register your executable directly using industry-standard explicit parameters
enable_testing()

register_gtest_target(sandbox
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/sandbox.cpp"
        "${BASE_SOURCE_PATH}/ipc/smart_file_descriptor/*.cpp"
    INCLUDES
        "${BASE_SOURCE_PATH}/ipc/"
        "${BASE_SOURCE_PATH}/ipc/smart_file_descriptor/"
        "${BASE_SOURCE_PATH}/"
    LIBRARIES
        Threads::Threads
        fmt::fmt
        GTest::gtest_main
)
