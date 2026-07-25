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

add_executable(english_challenge
    ${SOURCES}
)

# 3. Parse param and add to object
register_module_component_strict(english_challenge
    LIB_NOT_MERGE "0"
    BIN_APP_NAME english_challenge
    ROOT_DIR "${BASE_SOURCE_PATH}/application/english_apps"
    INSTALL_DESTINATION "haris/linux/ubuntu/application/english_apps"
    EXTRA_INCLUDES 
        "${BASE_SOURCE_PATH}/bin"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/haris/linux/common/logging"
        "${INTERNAL_ROOT_SEARCH_PATH}/include/haris/linux/common"
    EXTRA_LIBS
        "fmt::fmt"
    EXCLUDE_FILES
    EXCLUDE_DIRS
    ASSET_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/asset" # Clone all file in asset folder.
)

# Detail files.
# "${CMAKE_CURRENT_SOURCE_DIR}/asset/w1_english_word.txt" 

create_system_symlink_strict(english_challenge
    INSTALL_DESTINATION 
        "haris/linux/ubuntu/application/english_apps/english_challenge"
)