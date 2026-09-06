cmake_minimum_required(VERSION 3.10)
# 1. ONE-LINE BOOTSTRAP: Load the complete global ecosystem directly from your environment variable path
set(BASE_DIR_CONFIG "ubuntu")
include("$ENV{CMAKE_UTIL_PATH}/ProjectConfigInit.cmake")

# Load the modern automated framework builder
include(InstallUtil)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Find and target dependancies
find_package(fmt REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)

file(GLOB SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(termimage
    ${SOURCES}
)

# 3. Parse param and add to object
register_module_component_strict(termimage
    LIB_NOT_MERGE "0"
    BIN_APP_NAME termimage
    ROOT_DIR "${BASE_SOURCE_PATH}/application/termimage"
    INSTALL_DESTINATION "haris/linux/ubuntu/application/termimage"
    EXTRA_INCLUDES 
        "${BASE_SOURCE_PATH}/bin"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/haris/linux/common/logging"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/haris/linux/common"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/haris/linux/common"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/stb"
    EXTRA_LIBS
        "fmt::fmt"
    EXCLUDE_FILES
    EXCLUDE_DIRS
    ASSET_FILES
)

create_system_symlink_strict(termimage
    INSTALL_DESTINATION 
        "haris/linux/ubuntu/application/termimage/termimage"
)
