cmake_minimum_required(VERSION 3.12)
project(PosixPipeLibrary CXX)

# 1. ONE-LINE BOOTSTRAP: Load the complete global ecosystem directly from your environment variable path
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

include(InstallUtil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)

file(GLOB SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(ipc_pipe_obj OBJECT
    ${SOURCES}
)

target_link_libraries(ipc_pipe_obj PUBLIC fmt::fmt)

register_module_component_strict(ipc_pipe_obj
    LIB_NOT_MERGE
    STATIC_LIB_NAME posix_pipe                        # Explicitly passed now
    ROOT_DIR "${BASE_SOURCE_PATH}/ipc"
    INSTALL_DESTINATION "include/haris/linux/common/ipc"
    EXTRA_INCLUDES 
        "${BASE_SOURCE_PATH}/ipc/posix/pipe"
        "${BASE_SOURCE_PATH}"
    EXCLUDE_FILES
    EXCLUDE_DIRS
)