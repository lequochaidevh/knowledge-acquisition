cmake_minimum_required(VERSION 3.10)

project(PosixPipeProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/server.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../posix/pipe/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../posix/pipe/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../common/std17pch.h"
)

add_executable(server_pipe_test ${SOURCES})

find_package(Threads REQUIRED)
target_include_directories(server_pipe_test PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../posix/pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../common"
)
target_link_libraries(server_pipe_test PRIVATE Threads::Threads)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
