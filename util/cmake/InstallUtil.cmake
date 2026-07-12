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
    set(options LIB_NOT_MERGE)
    set(oneValueArgs ROOT_DIR INSTALL_DESTINATION STATIC_LIB_NAME)
    set(multiValueArgs EXTRA_INCLUDES EXCLUDE_FILES EXCLUDE_DIRS)

    # Parse arguments provided to the function call
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Strict Validation: Check ROOT_DIR
    if("${ARG_ROOT_DIR}" STREQUAL "")
        message(FATAL_ERROR "Missing mandatory argument: ROOT_DIR must be explicitly defined for target '${TARGET_NAME}'.")
    endif()

    # Strict Validation: Check INSTALL_DESTINATION
    if("${ARG_INSTALL_DESTINATION}" STREQUAL "")
        message(FATAL_ERROR "Missing mandatory argument: INSTALL_DESTINATION must be explicitly defined for target '${TARGET_NAME}'.")
    endif()

    # Strict Validation: Check EXTRA_INCLUDES
    if("${ARG_EXTRA_INCLUDES}" STREQUAL "")
        message(FATAL_ERROR "Missing mandatory argument: EXTRA_INCLUDES must contain at least one valid include path for target '${TARGET_NAME}'.")
    endif()

    # (EXCLUDE_FILES)
    if(ARG_EXCLUDE_FILES)
        list(REMOVE_ITEM ALL_SOURCES ${ARG_EXCLUDE_FILES})
    endif()

    # (EXCLUDE_DIRS)
    if(ARG_EXCLUDE_DIRS)
        set(FILTERED_SOURCES "")
        foreach(SRC_FILE ${ALL_SOURCES})
            set(SHOULD_EXCLUDE FALSE)
            foreach(DIR ${ARG_EXCLUDE_DIRS})
                # Kiểm tra nếu đường dẫn file bắt đầu bằng đường dẫn thư mục bị loại trừ
                if(SRC_FILE MATCHES "^${DIR}/")
                    set(SHOULD_EXCLUDE TRUE)
                    break()
                endif()
            endforeach()
            
            if(NOT SHOULD_EXCLUDE)
                list(APPEND FILTERED_SOURCES ${SRC_FILE})
            endif()
        endforeach()
        set(ALL_SOURCES ${FILTERED_SOURCES})
    endif()

    # Configure include directory boundaries universally
    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${ARG_ROOT_DIR}>
        $<BUILD_INTERFACE:${ARG_EXTRA_INCLUDES}>
        $<INSTALL_INTERFACE:include>
    )

    # Conditional Branching: Handle distinct static library creation vs. header accumulation
    if(ARG_LIB_NOT_MERGE)
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

        # Collect local headers and push them to the designated install path
        file(GLOB HEADERS_BE_INSTALLED "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
        install(FILES ${HEADERS_BE_INSTALLED}
            DESTINATION "${ARG_INSTALL_DESTINATION}"
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
endfunction()