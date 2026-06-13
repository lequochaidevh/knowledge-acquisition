cmake_minimum_required(VERSION 3.10)

project(PosixPipeProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Read minor root
# Check if the external environment variable exists
if(DEFINED ENV{LOCAL_MINOR_ROOT})
    # Read the environment variable and force it into CMAKE_INSTALL_PREFIX
    set(LOGGING_SEARCH_PATH "$ENV{LOCAL_MINOR_ROOT}")
    set(CMAKE_INSTALL_PREFIX "$ENV{LOCAL_MINOR_ROOT}" CACHE PATH "Install path from env" FORCE)
    message(STATUS "--> Found LOGGING_SEARCH_PATH to: ${LOGGING_SEARCH_PATH}")
    message(STATUS "--> Found environment path. Installing to: ${CMAKE_INSTALL_PREFIX}")
else()
    # Fallback default path if the environment variable is not exported
    message(STATUS "--> Environment variable not found. Using default: ${CMAKE_INSTALL_PREFIX}")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/local_minor_root")    
    endif()
endif()

file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../posix/pipe/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../posix/pipe/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../common/std17pch.h"
)

message(STATUS "--> Searching for logging library in: ${LOGGING_SEARCH_PATH}")
# 2. Find Header and logging static library in the path
find_path(LOGGING_INCLUDE_DIR 
    NAMES logger.h
    PATHS ${LOGGING_SEARCH_PATH}/include
    NO_DEFAULT_PATH
)

find_library(LOGGING_LIBRARY 
    NAMES logging
    PATHS ${LOGGING_SEARCH_PATH}/lib
    NO_DEFAULT_PATH
)

find_package(fmt REQUIRED PATHS ${LOGGING_SEARCH_PATH} NO_DEFAULT_PATH)

add_executable(client_pipe_test ${SOURCES})

find_package(Threads REQUIRED)

message(STATUS "--> Searching for LOGGING_LIBRARY in: ${LOGGING_LIBRARY}")

target_include_directories(client_pipe_test PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../posix/pipe"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../common"
    ${LOGGING_INCLUDE_DIR}
)
target_link_libraries(client_pipe_test PRIVATE 
    Threads::Threads
    ${LOGGING_LIBRARY} 
    fmt::fmt
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
