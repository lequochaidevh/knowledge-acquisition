cmake_minimum_required(VERSION 3.10)

project(PosixPipeProject LANGUAGES CXX)
# project(LinuxMultiPlatformProject VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(client)
add_subdirectory(server)

