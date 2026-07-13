cmake_minimum_required(VERSION 3.12)
project(PosixPipeLibrary CXX)

# 1. Load the complete global ecosystem directly from your environment variable path
set(BASE_DIR_CONFIG "common")
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

include(InstallUtil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find and target dependancies
find_package(nlohmann_json REQUIRED PATHS "${INTERNAL_ROOT_SEARCH_PATH}" NO_DEFAULT_PATH)

file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(json_helper_object OBJECT
    ${SOURCES}
)

# 3. Parse param and add to object
register_module_component_strict(json_helper_object
    LIB_NOT_MERGE "${LIB_NOT_MERGE}"
    STATIC_LIB_NAME json_helper
    ROOT_DIR "${BASE_SOURCE_PATH}/util"
    INSTALL_DESTINATION "include/haris/linux/common/util/helper/nlohmann_json"
    EXTRA_INCLUDES
        "${BASE_SOURCE_PATH}/util"
        "${BASE_SOURCE_PATH}/util/helper/nlohmann_json"
    EXTRA_LIBS
        "nlohmann_json::nlohmann_json"
    EXCLUDE_FILES
    EXCLUDE_DIRS
)
