cmake_minimum_required(VERSION 3.10)

project(PosixSocketProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Read minor root
# Check if the external environment variable exists
if(DEFINED ENV{LOCAL_MINOR_ROOT})
    # Read the environment variable and force it into CMAKE_INSTALL_PREFIX
    set(ROOT_SEARCH_PATH "$ENV{LOCAL_MINOR_ROOT}")
    # set(LOGGING_LIBRARY "$ENV{LOCAL_MINOR_ROOT}")
    set(CMAKE_INSTALL_PREFIX "$ENV{LOCAL_MINOR_ROOT}" CACHE PATH "Install path from env" FORCE)
    message(STATUS "--> Found ROOT_SEARCH_PATH to: ${ROOT_SEARCH_PATH}")
    message(STATUS "--> Found environment path. Installing to: ${CMAKE_INSTALL_PREFIX}")
else()
    # Fallback default path if the environment variable is not exported
    message(STATUS "--> Environment variable not found. Using default: ${CMAKE_INSTALL_PREFIX}")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/local_minor_root")    
    endif()
endif()

file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/server.cpp"
)

message(STATUS "--> Searching for logging library in: ${ROOT_SEARCH_PATH}")
# 2. Find Header and logging static library in the path
find_path(LOGGING_INCLUDE_DIR 
    NAMES logging
    PATHS ${ROOT_SEARCH_PATH}/include
    NO_DEFAULT_PATH
)

find_library(LOGGING_LIBRARY 
    NAMES logging
    PATHS ${ROOT_SEARCH_PATH}/lib
    NO_DEFAULT_PATH
)

find_path(COMMON_IPC_INCLUDE_DIR
    NAMES haris/linux/common/ipc
    PATHS ${ROOT_SEARCH_PATH}/include
    NO_DEFAULT_PATH
)

if(LIB_NOT_MERGE)
    # -lunix_socket
    find_library(COMMON_IPC_LIBRARY 
        NAMES unix_socket
        PATHS ${ROOT_SEARCH_PATH}/lib
        NO_DEFAULT_PATH
    )
else()
    # -lcommon_ipc
    find_library(COMMON_IPC_LIBRARY 
        NAMES common_ipc
        PATHS ${ROOT_SEARCH_PATH}/lib
        NO_DEFAULT_PATH
    )
endif()

find_package(fmt REQUIRED PATHS ${ROOT_SEARCH_PATH} NO_DEFAULT_PATH)

add_executable(server_socket_test ${SOURCES})

find_package(Threads REQUIRED)

message(STATUS "--> Searching for LOGGING_LIBRARY in: ${LOGGING_LIBRARY}")
message(STATUS "--> Searching for LOGGING_INCLUDE_DIR in: ${LOGGING_INCLUDE_DIR}")
message(STATUS "--> Searching for COMMON_IPC_INCLUDE_DIR in: ${COMMON_IPC_INCLUDE_DIR}/haris/linux/common/ipc")
message(STATUS "--> Searching for COMMON_IPC_LIBRARY in: ${COMMON_IPC_LIBRARY}")

target_include_directories(server_socket_test PRIVATE
    "${COMMON_IPC_INCLUDE_DIR}/haris/linux/common/ipc"
    "${LOGGING_INCLUDE_DIR}"
)

target_link_libraries(server_socket_test PRIVATE
    Threads::Threads
    ${COMMON_IPC_LIBRARY} 
    ${LOGGING_LIBRARY}
    fmt::fmt
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
