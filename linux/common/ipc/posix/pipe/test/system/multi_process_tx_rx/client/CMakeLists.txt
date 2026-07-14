cmake_minimum_required(VERSION 3.10)
project(PosixPipeProject LANGUAGES CXX)

# 1. ONE-LINE BOOTSTRAP: Load the complete global ecosystem directly from your environment variable path
set(BASE_DIR_CONFIG "common")
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

# Load the modern automated framework builder
include(GTestSetup)

# 2. Register your executable directly using industry-standard explicit parameters
enable_testing()

register_gtest_target(client
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/client.cpp"
        "${BASE_SOURCE_PATH}/ipc/posix/pipe/*.cpp"
    INCLUDES
        "${BASE_SOURCE_PATH}/"
        "${BASE_SOURCE_PATH}/ipc/"
        "${BASE_SOURCE_PATH}/ipc/posix/pipe/"
    LIBRARIES
        Threads::Threads
        fmt::fmt
        GTest::gtest_main
        "${INTERNAL_ROOT_SEARCH_PATH}/lib/libcommand_util.a"
)
