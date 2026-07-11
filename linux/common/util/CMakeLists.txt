
cmake_minimum_required(VERSION 3.12)
project(LinuxCommon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UTIL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB IPC_INSTALL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

install(FILES ${IPC_INSTALL_HEADERS}
    DESTINATION include/haris/linux/common/util
)
