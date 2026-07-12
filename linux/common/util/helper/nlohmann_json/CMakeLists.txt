cmake_minimum_required(VERSION 3.12)
project(JsonHelperLibrary CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if the external environment variable exists
if(DEFINED ENV{MINOR_ROOT_PATH})
    set(MINOR_ROOT_PATH "$ENV{MINOR_ROOT_PATH}")
    message(STATUS "Found environment path.")
    message(STATUS "MINOR_ROOT_PATH: ${MINOR_ROOT_PATH}")

    set(CMAKE_INSTALL_PREFIX "$ENV{MINOR_ROOT_PATH}" CACHE PATH "Install path from env" FORCE)
    set(CMAKE_PREFIX_PATH "$ENV{MINOR_ROOT_PATH}")
    message(STATUS "--> Found environment path. Configure cmake prefix to: ${CMAKE_PREFIX_PATH}")
    message(STATUS "--> Found environment path. Installing to: ${CMAKE_INSTALL_PREFIX}")
else()
    message(FATAL_ERROR "--> Environment variable not found.")
endif()

add_library(json_helper STATIC json_helper.cpp json_helper.h)

find_package(nlohmann_json REQUIRED PATHS "${MINOR_ROOT_PATH}" NO_DEFAULT_PATH)

target_link_libraries(json_helper PUBLIC 
        nlohmann_json::nlohmann_json)

# Configure include directory paths for building and installing
target_include_directories(json_helper PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 3. CONFIGURE INSTALLATION TARGETS
# Install the static library binary (libjson_helper.a) to 'lib/'
install(TARGETS json_helper
    ARCHIVE DESTINATION lib
)

# Install the header file (logger.h) to 'include/'
install(FILES json_helper.h
    DESTINATION include/haris/linux/common/util/helper/nlohmann_json
)
