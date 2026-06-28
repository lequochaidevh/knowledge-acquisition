
cmake_minimum_required(VERSION 3.12)
project(LinuxCommon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(helper)
add_subdirectory(ipc)
add_subdirectory(logging)

# Install the header file (std17pch.h) to 'include/'
install(FILES std17pch.h
    DESTINATION include
)

