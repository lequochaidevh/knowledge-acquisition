# ==============================================================================
# Shared Macro: register_gtest_executable
# Description: Automates boilerplate for registering a GoogleTest binary
# Arguments:
#   - TARGET_NAME : Name of the output test binary (e.g., sandbox)
#   - SOURCES     : List of source files specific to this test
#   - INCLUDES    : List of include directories specific to this test
# ==============================================================================
function(register_gtest_target TARGET_NAME)
    # 1. Define valid argument keywords for the function parsing engine
    set(OPTIONS "")
    set(ONE_VALUE_ARGS "")
    set(MULTI_VALUE_ARGS SOURCES INCLUDES LIBRARIES) # Added LIBRARIES here

    # 2. Parse the arguments passed to the function
    cmake_parse_arguments(ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN})

    # 3. AUTOMATIC GLOBBING: Process wildcard paths in SOURCES
    set(RESOLVED_SOURCES "")
    foreach(SOURCE_ENTRY ${ARG_SOURCES})
        if("${SOURCE_ENTRY}" MATCHES "\\*")
            file(GLOB GLOBBED_FILES "${SOURCE_ENTRY}")
            list(APPEND RESOLVED_SOURCES ${GLOBBED_FILES})
        else()
            list(APPEND RESOLVED_SOURCES "${SOURCE_ENTRY}")
        endif()
    endforeach()

    # Error check: Ensure that at least one source file was resolved
    if(NOT RESOLVED_SOURCES)
        message(FATAL_ERROR "[GTest Engine] No source files found or resolved for target '${TARGET_NAME}'!")
    endif()

    # 4. Locate system dependencies and third-party libraries
    find_path(LOGGING_INCLUDE_DIR 
        NAMES logger.h 
        PATHS ${INTERNAL_ROOT_SEARCH_PATH}/include/logging 
        NO_DEFAULT_PATH
    )
    
    find_library(LOGGING_LIBRARY 
        NAMES logging 
        PATHS ${INTERNAL_ROOT_SEARCH_PATH}/lib 
        NO_DEFAULT_PATH
    )
    
    find_package(fmt REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)
    find_package(nlohmann_json REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)
    find_package(GTest REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)
    find_package(Threads REQUIRED)

    # 5. Compile the targeted test binary executable using resolved sources
    add_executable(${TARGET_NAME} ${RESOLVED_SOURCES})

    # 6. Apply include pathways
    target_include_directories(${TARGET_NAME} PRIVATE 
        ${LOGGING_INCLUDE_DIR}
        ${ARG_INCLUDES}
    )

    # 7. Bind core libraries and any target-specific custom LIBRARIES passed in
    target_link_libraries(${TARGET_NAME} PRIVATE 
        ${LOGGING_LIBRARY} 
        ${ARG_LIBRARIES} # Custom libraries are linked here
    )

    # 8. Output runtime binary directly into local calling test directory
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BIN_DIR}"
    )

    # 9. Scan and inject test macros to the system test discovery register
    include(GoogleTest)
    gtest_discover_tests(${TARGET_NAME})

endfunction()
