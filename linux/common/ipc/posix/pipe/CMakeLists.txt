cmake_minimum_required(VERSION 3.12)
project(PosixPipeLibrary CXX)

# 1. Load the complete global ecosystem directly from your environment variable path
set(BASE_DIR_CONFIG "common")
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

include(InstallUtil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find and target dependancies
find_package(fmt REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)

file(GLOB SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(pipe_object OBJECT
    ${SOURCES}
)

# 3. Parse param and add to object
register_module_component_strict(pipe_object
    LIB_NOT_MERGE "${LIB_NOT_MERGE}"
    STATIC_LIB_NAME posix_pipe
    ROOT_DIR "${BASE_SOURCE_PATH}/ipc"
    INSTALL_DESTINATION "include/haris/linux/common/ipc"
    EXTRA_INCLUDES 
        "${BASE_SOURCE_PATH}/ipc/posix/pipe"
        "${BASE_SOURCE_PATH}"
        "${BASE_SOURCE_PATH}/logging"
    EXTRA_LIBS
        "fmt::fmt"
    EXCLUDE_FILES
    EXCLUDE_DIRS
)