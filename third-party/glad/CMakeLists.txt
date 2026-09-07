cmake_minimum_required(VERSION 3.15)
project(glad LANGUAGES C)

if(NOT DEFINED CMAKE_INSTALL_PREFIX AND DEFINED ENV{LOCAL_MINOR_ROOT})
    set(CMAKE_INSTALL_PREFIX "$ENV{LOCAL_MINOR_ROOT}" CACHE PATH "Install prefix" FORCE)
endif()

add_library(glad STATIC src/glad.c)

target_include_directories(glad PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})

install(TARGETS glad
    EXPORT gladTargets
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT gladTargets
    FILE gladConfig.cmake
    NAMESPACE glad::
    DESTINATION lib/cmake/glad
)