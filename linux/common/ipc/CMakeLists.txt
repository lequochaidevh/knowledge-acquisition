
cmake_minimum_required(VERSION 3.12)
project(LinuxCommon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IPC_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(posix/pipe)
# add_subdirectory(unix/socket)

if(LIB_NOT_MERGE)

    file(GLOB COMMON_IPC_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )
    # Copies the 'ipc_common' folder and its contents into 'include/'
    install(FILES ${IPC_INSTALL_HEADERS} ${COMMON_IPC_HEADERS}
        DESTINATION include/haris/linux/common/ipc
    )
else()

    add_library(common_ipc STATIC
        $<TARGET_OBJECTS:ipc_pipe_obj>
        $<TARGET_OBJECTS:ipc_socket_obj>
    )

    # 3. CONFIGURE INSTALLATION TARGETS
    # Install the static library binary (libposix_pipe.a) to 'lib/'
    install(TARGETS common_ipc
        ARCHIVE DESTINATION lib
    )

    file(GLOB COMMON_IPC_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )

    # Copies the 'ipc_common' folder and its contents into 'include/'
    install(FILES ${IPC_INSTALL_HEADERS} ${COMMON_IPC_HEADERS}
        DESTINATION include/haris/linux/common/ipc
    )

endif()