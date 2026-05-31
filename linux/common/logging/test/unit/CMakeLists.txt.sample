cmake_minimum_required(VERSION 3.12)
project(LoggingTest CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Read minor root
if(DEFINED ENV{LOCAL_MINOR_ROOT})
    set(LOGGING_SEARCH_PATH "$ENV{LOCAL_MINOR_ROOT}")
    set(CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")
    message(STATUS "--> Found environment path. Configure cmake prefix to: ${CMAKE_PREFIX_PATH}")
else()
    # fallback
    set(LOGGING_SEARCH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/local_minor_root")
endif()

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

find_package(fmt REQUIRED)

# Find the library
if(NOT LOGGING_INCLUDE_DIR OR NOT LOGGING_LIBRARY)
    message(FATAL_ERROR "Could not find logging library or header at ${LOGGING_SEARCH_PATH}")
endif()

# 3. Target executable
add_executable(logging_test 
    main.cpp 
    module_a.cpp 
    module_b.cpp
)

# 4. Set Include path
target_include_directories(logging_test PRIVATE ${LOGGING_INCLUDE_DIR})

# 5. Link to static libraries installed (liblogging.a)
target_link_libraries(logging_test PRIVATE 
    ${LOGGING_LIBRARY} 
    fmt::fmt
)
