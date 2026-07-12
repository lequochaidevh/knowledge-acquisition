# ==============================================================================
# Module: ProjectConfigInit
# Description: Global bootstrap environment configuration shared by all CMake projects
# ==============================================================================

# 1. Force C++ Standard configuration globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Read minor root from host environment variables
if(DEFINED ENV{MINOR_ROOT_PATH})
    set(INTERNAL_ROOT_SEARCH_PATH "$ENV{MINOR_ROOT_PATH}") 
    set(CMAKE_INSTALL_PREFIX "$ENV{MINOR_ROOT_PATH}" CACHE PATH "Install path from env" FORCE)
    message(STATUS "[INIT] Bound INTERNAL_ROOT_SEARCH_PATH to: ${INTERNAL_ROOT_SEARCH_PATH}")
else()
    message(FATAL_ERROR "[INIT] Critical Environment variable 'MINOR_ROOT_PATH' not found!")
endif()

# 3. Explicitly update CMAKE_MODULE_PATH so CMake can locate companion files
set(CMAKE_UTIL_PATH "$ENV{CMAKE_UTIL_PATH}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_UTIL_PATH}")

# 4. Load core pathfinder utilities and discover source workspace root
include(FindParentDirectory)
find_parent_directory("common" BASE_SOURCE_PATH)
message(STATUS "[INIT] BASE_SOURCE_PATH is verified at: ${BASE_SOURCE_PATH}")

# # 5. Load the primary GTest test framework generator macro engine
# include(GTestSetup)
