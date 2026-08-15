cmake_minimum_required(VERSION 3.16)
project(GStreamerApp LANGUAGES CXX)

# Force C++17 or C++20 standard as needed
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Look for GStreamer using pkg-config
find_package(PkgConfig REQUIRED)

# 2. Instruct pkg-config to find our local GStreamer modules
# This detects the dependencies from your $LOCAL_MINOR_ROOT installation
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED IMPORTED_TARGET gstreamer-video-1.0)

# Define your source files
set(SOURCE_FILES main.cpp)

# Define your executable output
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 3. Link the local GStreamer modules to your executable target
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::GSTREAMER
        PkgConfig::GST_APP
        PkgConfig::GST_VIDEO
)