cmake_minimum_required(VERSION 3.12)
project(UnixSocketLibrary CXX)

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

file(GLOB SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(ipc_socket_obj OBJECT
    ${SOURCES}
)

target_link_libraries(ipc_socket_obj PUBLIC fmt::fmt)

if("${IPC_ROOT_DIR}" STREQUAL "")
    message(WARNING "IPC_ROOT_DIR is empty will auto find it")
    set(IPC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../")
    message(WARNING "IPC_ROOT_DIR = ${IPC_ROOT_DIR}")
endif()

# Configure include directory paths for building and installing
target_include_directories(ipc_socket_obj PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../>
    $<INSTALL_INTERFACE:include>
    ${IPC_ROOT_DIR}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

if(LIB_NOT_MERGE)
    add_library(unix_socket STATIC
        $<TARGET_OBJECTS:ipc_socket_obj>
    )

    # 3. CONFIGURE INSTALLATION TARGETS
    # Install the static library binary (libunix_socket.a) to 'lib/'
    install(TARGETS unix_socket
        ARCHIVE DESTINATION lib
    )

    file(GLOB HEADERS_BE_INSTALLED
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )

    # Copies the 'posix' folder and its contents into 'include/'
    install(FILES ${HEADERS_BE_INSTALLED}
        DESTINATION include/haris/linux/common/ipc
    )
else() 
    file(GLOB PIPE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wshadow -Werror)
    endif()

    set(IPC_INSTALL_HEADERS
        ${IPC_INSTALL_HEADERS}
        ${PIPE_HEADERS}
        PARENT_SCOPE
    )
endif()
