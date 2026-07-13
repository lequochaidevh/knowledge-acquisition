include(CMakeParseArguments)

#[[
# @brief Universally registers a module target with strict parameter enforcement.
# @note Throws a FATAL_ERROR if any mandatory arguments are missing or empty.
# @param TARGET_NAME The object library target to configure.
# @param LIB_NOT_MERGE (Option) Flag to trigger separate static library creation and installation.
# @param ROOT_DIR (OneValue) [MANDATORY] Custom root directory for module headers.
# @param INSTALL_DESTINATION (OneValue) [MANDATORY] Directory path for header installation.
# @param STATIC_LIB_NAME (OneValue) [MANDATORY if LIB_NOT_MERGE is active] Output static library name.
# @param EXTRA_INCLUDES (MultiValue) [MANDATORY] List of additional include paths to append.
#]]
function(register_module_component_strict TARGET_NAME)
    # Define generic argument keywords for the parser
    set(options "")
    set(oneValueArgs ROOT_DIR INSTALL_DESTINATION STATIC_LIB_NAME LIB_NOT_MERGE)
    set(multiValueArgs EXTRA_INCLUDES EXTRA_LIBS EXCLUDE_FILES EXCLUDE_DIRS)

    # Parse arguments provided to the function call
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Strict Validation: Check ROOT_DIR
    if("${ARG_ROOT_DIR}" STREQUAL "")
        message(FATAL_ERROR "Missing mandatory argument: ROOT_DIR must be explicitly defined for target '${TARGET_NAME}'.")
    endif()

    # 1. Scan all header files recursively (This returns ABSOLUTE paths)
    file(GLOB_RECURSE ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

    # 2. Create an empty list to store the filtered files
    set(FINAL_HEADERS_TO_INSTALL "")
    set(HAVE_EXCLUDED "")
    # --- DEBUG LINES: Let's see what CMake actually receives ---
    message(STATUS "[DEBUG] ARG_EXCLUDE_DIRS contains: '${ARG_EXCLUDE_DIRS}'")

    # 3. Convert excluded files list to ABSOLUTE paths for exact matching
    set(ABS_EXCLUDE_FILES "")
    foreach(EX_FILE ${ARG_EXCLUDE_FILES})
        get_filename_component(ABS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${EX_FILE}" ABSOLUTE)
        list(APPEND ABS_EXCLUDE_FILES ${ABS_FILE})
    endforeach()

    # 4. Fallback Bulletproof Filter
    foreach(SOURCE ${ALL_SOURCES})
        set(SHOULD_EXCLUDE FALSE)

        # Check 1: File exclusion
        if("${SOURCE}" IN_LIST ABS_EXCLUDE_FILES)
            set(SHOULD_EXCLUDE TRUE)
        endif()

        # Check 2: Slash-agnostic directory kill-switch
            if(NOT SHOULD_EXCLUDE)
                # Normalize source path to forward slashes
                file(TO_CMAKE_PATH "${SOURCE}" SOURCE_NORMALIZED)

                foreach(DIR ${ARG_EXCLUDE_DIRS})
                    # SANITIZE USER INPUT: Strip any leading or trailing slashes from the argument
                    string(REGEX REPLACE "^/|/$" "" CLEAN_DIR "${DIR}")

                    # Match precise folder boundaries (handles /sfd/ anywhere in path safely)
                    if("${SOURCE_NORMALIZED}" MATCHES "(^|/)${CLEAN_DIR}(/|$)")
                        message(STATUS "[DEBUG] SUCCESS! EXCLUDING FILE: ${SOURCE}")
                        set(SHOULD_EXCLUDE TRUE)
                        break()
                    endif()
                endforeach()
            endif()

            if(NOT SHOULD_EXCLUDE)
                list(APPEND FINAL_HEADERS_TO_INSTALL ${SOURCE})
            endif()
        endforeach()

    # Configure include directory boundaries universally
    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
        ${ARG_ROOT_DIR}
        ${ARG_EXTRA_INCLUDES}
    )

    target_link_libraries(${TARGET_NAME} PUBLIC ${ARG_EXTRA_LIBS})

    # Conditional Branching: Handle distinct static library creation vs. header accumulation
    if(${ARG_LIB_NOT_MERGE} STREQUAL "1")
        # Strict Validation for conditional argument: STATIC_LIB_NAME must be provided if LIB_NOT_MERGE is set
        if("${ARG_STATIC_LIB_NAME}" STREQUAL "")
            message(FATAL_ERROR "Missing mandatory argument: STATIC_LIB_NAME must be specified when 'LIB_NOT_MERGE' is enabled for target '${TARGET_NAME}'.")
        endif()

        # Compile the standalone static library using the user-defined name
        add_library(${ARG_STATIC_LIB_NAME} STATIC $<TARGET_OBJECTS:${TARGET_NAME}>)

        # Rule for deploying the .a static binary file
        install(TARGETS ${ARG_STATIC_LIB_NAME}
            ARCHIVE DESTINATION lib
        )

    else()
        # Collect headers to export upwards into parent visibility scope
        file(GLOB LOCAL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
        set(MODULE_INSTALL_HEADERS
            ${MODULE_INSTALL_HEADERS}
            ${LOCAL_HEADERS}
            PARENT_SCOPE
        )
    endif()

    # Collect local headers and push them to the designated install path
    install(FILES ${FINAL_HEADERS_TO_INSTALL}
        DESTINATION "${ARG_INSTALL_DESTINATION}"
    )
endfunction()