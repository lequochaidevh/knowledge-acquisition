cmake_minimum_required(VERSION 3.16)
project(robot2dof)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Link ---
find_package(Threads REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "$ENV{LOCAL_MINOR_ROOT}")

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

file(GLOB SOURCE_FILES_DIR
    "${CMAKE_SOURCE_DIR}/robot/2DOF/*.cpp"
    "${CMAKE_SOURCE_DIR}/window/*.cpp"
    "${CMAKE_SOURCE_DIR}/gcode/*.cpp"
)
# --- Robot2DOF App ---
add_executable(robot2dof
    application.cpp
    ${SOURCE_FILES_DIR}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/base>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/robot/2DOF>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/window>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gcode>
    $<INSTALL_INTERFACE:include/robot2dof>
)

target_compile_features(robot2dof PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(robot2dof PRIVATE -O3 -march=native)
endif()

target_link_libraries(robot2dof PRIVATE 
    glad::glad 
    glfw 
    OpenGL::GL 
    glm::glm
    Threads::Threads
)
