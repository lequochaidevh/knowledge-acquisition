cmake_minimum_required(VERSION 3.12)
project(LoggingLibrary CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if the external environment variable exists
if(DEFINED ENV{LOCAL_MINOR_ROOT})
    # Read the environment variable and force it into CMAKE_INSTALL_PREFIX
    set(CMAKE_INSTALL_PREFIX "$ENV{LOCAL_MINOR_ROOT}" CACHE PATH "Install path from env" FORCE)
    set(CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")
    message(STATUS "--> Found environment path. Configure cmake prefix to: ${CMAKE_PREFIX_PATH}")
    message(STATUS "--> Found environment path. Installing to: ${CMAKE_INSTALL_PREFIX}")
else()
    # Fallback default path if the environment variable is not exported
    message(STATUS "--> Environment variable not found. Using default: ${CMAKE_INSTALL_PREFIX}")
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 
        set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/local_minor_root")    
    endif()
endif()


find_package(fmt REQUIRED PATHS ${CMAKE_PREFIX_PATH} NO_DEFAULT_PATH)

add_library(logging STATIC logger.cpp logger.h)

target_link_libraries(logging PUBLIC fmt::fmt)

# Configure include directory paths for building and installing
target_include_directories(logging PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../
    $<INSTALL_INTERFACE:include>
)

# 3. CONFIGURE INSTALLATION TARGETS
# Install the static library binary (liblogging.a) to 'lib/'
install(TARGETS logging
    ARCHIVE DESTINATION lib
)

# Install the header file (logger.h) to 'include/'
install(FILES logger.h
    DESTINATION include/logging
)
