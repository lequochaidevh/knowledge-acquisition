cmake_minimum_required(VERSION 3.12)
project(LoggingLibrary CXX)

# 1. Load the complete global ecosystem directly from your environment variable path
set(BASE_DIR_CONFIG "common")
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

include(InstallUtil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find and set variable to declare dependacies packages.
find_package(fmt REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)

# 3. Configure include directory paths for building and installing
file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(logging_object OBJECT
    ${SOURCES}
)

set(CONFIG_LOGGING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/logging_config.json")

target_compile_definitions(logging_object PRIVATE
    CONFIG_LOGGING_PATH="${CONFIG_LOGGING_PATH}"
)

register_module_component_strict(logging_object
    LIB_NOT_MERGE 1 # Not merge lib
    STATIC_LIB_NAME logging
    ROOT_DIR "${BASE_SOURCE_PATH}/logging"
    INSTALL_DESTINATION "include/haris/linux/common/logging"
    EXTRA_INCLUDES
        "${BASE_SOURCE_PATH}"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/haris/linux/common/util"
    EXTRA_LIBS
        "fmt::fmt"
    EXCLUDE_FILES
    EXCLUDE_DIRS
)
