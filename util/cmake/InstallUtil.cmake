include(CMakeParseArguments)

#[[
# @brief Universally registers and provisions a module target with strict parameter enforcement.
# @details Automates header filtering, target inclusion, architectural linking, and conditional 
#          standalone static library compilation or application bundling alongside static asset tracking.
# @note Throws a FATAL_ERROR if any mandatory or conditionally required arguments are missing or empty.
#
# @param TARGET_NAME The primary object library target to configure.
#
# @param LIB_NOT_MERGE (OneValue) Boolean flag ("1" or "0") to trigger separate static library creation and installation.
# @param ROOT_DIR (OneValue) [MANDATORY] Custom root directory for mapping and locating module header boundaries.
# @param INSTALL_DESTINATION (OneValue) [MANDATORY] Directory path location where filtered headers are deployed.
# @param STATIC_LIB_NAME (OneValue) [MANDATORY if LIB_NOT_MERGE is active] Output name for the compiled static library (.a/.lib).
# @param BIN_APP_NAME (OneValue) [OPTIONAL] Designated name for the compiled tool or companion binary executable.
#
# @param EXTRA_INCLUDES (MultiValue) List of additional compiler include pathways to append to the target interface.
# @param EXTRA_LIBS (MultiValue) List of external static, shared, or interface libraries to link into the target.
# @param EXCLUDE_FILES (MultiValue) Specific file paths relative to the current source directory to omit from installation.
# @param EXCLUDE_DIRS (MultiValue) Specific directory names anywhere in the tree to block from recursive scanning.
# @param ASSET_FILES (MultiValue) Collection of static resource files or folder paths to track and ship with the package.
#]]
function(register_module_component_strict TARGET_NAME)
    # Define generic argument keywords for the parser
    set(options "")
    set(oneValueArgs ROOT_DIR INSTALL_DESTINATION STATIC_LIB_NAME BIN_APP_NAME LIB_NOT_MERGE)
    set(multiValueArgs EXTRA_INCLUDES EXTRA_LIBS EXCLUDE_FILES EXCLUDE_DIRS ASSET_FILES)

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

    # Install application
    # Stage physical application files to system configurations
    if("${ARG_BIN_APP_NAME}" STREQUAL "")
            message("The installation proceeded without the BIN file.")
    else()
        install(TARGETS ${ARG_BIN_APP_NAME}
            RUNTIME DESTINATION "${ARG_INSTALL_DESTINATION}"
        )
    endif()
    
    # Handle directory cloning with strict trailing-slash normalization
    if(ARG_ASSET_FILES)
        foreach(ASSET_PATH IN LISTS ARG_ASSET_FILES)
            # Normalize the path to remove any trailing slashes safely
            get_filename_component(NORMALIZED_PATH "${ASSET_PATH}" ABSOLUTE)

            if(IS_DIRECTORY "${NORMALIZED_PATH}")
                # Extract the pure directory folder name (e.g., "asset")
                get_filename_component(DIR_NAME "${NORMALIZED_PATH}" NAME)

                # Clone the directory as a standalone folder inside the target binary directory
                add_custom_command(
                    TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${NORMALIZED_PATH}"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/${DIR_NAME}"
                    COMMENT "Cloning asset folder structure: ${DIR_NAME}"
                )
                # Install the entire directory into the installation destination
                # Note: NOT adding a trailing slash ensures the folder itself is copied
                install(DIRECTORY "${NORMALIZED_PATH}"
                    DESTINATION "${ARG_INSTALL_DESTINATION}"
                )
            else()
                # Safe fallback configuration for mixed file inputs
                get_filename_component(ASSET_FILENAME "${NORMALIZED_PATH}" NAME)
                add_custom_command(
                    TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${NORMALIZED_PATH}"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ASSET_FILENAME}"
                )
                # Copy each individual file to the binary output directory after build
                add_custom_command(
                    TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ASSET_PATH}"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ASSET_FILENAME}"
                    COMMENT "Copying asset: ${ASSET_FILENAME}"
                )
                # Install individual asset files directly to the installation destination
                install(FILES "${NORMALIZED_PATH}"
                    DESTINATION "${ARG_INSTALL_DESTINATION}"
                )
            endif()
        endforeach()
    endif()
endfunction()

#[[
# @brief Universally provisions a system-wide symbolic link for a targeted binary executable.
# @details Automates symlink generation pointing to either local build paths or production deployment layouts.
# @note Throws a FATAL_ERROR if critical mapping arguments are omitted.
#
# @param TARGET_NAME The reference executable target linked to this symlink.
# @param INSTALL_DESTINATION (OneValue) [MANDATORY] The absolute path to the actual binary file executable.
#
# @param LINK_NAME (OneValue) [OPTIONAL] Custom name for the symlink. Defaults to target name if empty.
# @param LINK_DESTINATION (OneValue) [OPTIONAL] System directory for the symlink. Defaults to '/usr/local/bin' if empty.
# @param DEPLOY_STAGE (OneValue) [OPTIONAL] Stage to create symlink: 'INSTALL' (default, requires sudo) or 'POST_BUILD'.
#]]
function(create_system_symlink_strict TARGET_NAME)
    # 1. Define exact keyword boundaries for clean code interface
    set(options "")
    set(oneValueArgs INSTALL_DESTINATION LINK_NAME LINK_DESTINATION DEPLOY_STAGE)
    set(multiValueArgs "")

    # 2. Parse arguments fed to the public engine call
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 3. Strict Validation: Protect symlink layout from blank source paths
    if("${ARG_INSTALL_DESTINATION}" STREQUAL "")
        message(FATAL_ERROR "[Symlink Error] Missing mandatory argument: 'INSTALL_DESTINATION' must be explicitly set for target '${TARGET_NAME}'.")
    endif()

    # 4. Apply smart defaults for optional arguments
    if("${ARG_LINK_NAME}" STREQUAL "")
        set(ARG_LINK_NAME "${TARGET_NAME}")
    endif()

    if("${ARG_LINK_DESTINATION}" STREQUAL "")
        set(ARG_LINK_DESTINATION "/usr/local/bin")
    endif()

    if("${ARG_DEPLOY_STAGE}" STREQUAL "")
        set(ARG_DEPLOY_STAGE "INSTALL")
    endif()

    # 5. Define final target symlink absolute path
    set(FINAL_SYMLINK_PATH "${ARG_LINK_DESTINATION}/${ARG_LINK_NAME}")

    # 6. Branching Logic based on desired Deployment Stage
    if("${ARG_DEPLOY_STAGE}" STREQUAL "INSTALL")
        # --- STAGE 1: SYSTEM INSTALLATION ROUTINE (Executed during 'make install' / 'sudo make install') ---
        install(CODE "
            message(STATUS \"[Symlink Deployment] Creating system symlink: ${FINAL_SYMLINK_PATH} -> ${ARG_INSTALL_DESTINATION}\")
            
            # Execute cross-platform CMake symlink creation command within the installer shell
            execute_process(
                COMMAND \${CMAKE_COMMAND} -E create_symlink \"${INTERNAL_ROOT_SEARCH_PATH}/${ARG_INSTALL_DESTINATION}\" \"${FINAL_SYMLINK_PATH}\"
                RESULT_VARIABLE SYMLINK_RESULT
            )
            
            # Validate runtime execution safety
            if(NOT SYMLINK_RESULT EQUAL 0)
                message(WARNING \"[Symlink Warning] Failed to create symlink at '${FINAL_SYMLINK_PATH}'. Verify root privileges (sudo).\")
            endif()
        " COMPONENT Runtime)

    elseif("${ARG_DEPLOY_STAGE}" STREQUAL "POST_BUILD")
        # --- STAGE 2: LOCAL DEVELOPMENT ROUTINE (Executed immediately after compilation completes) ---
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${INTERNAL_ROOT_SEARCH_PATH}/${ARG_INSTALL_DESTINATION}" "${FINAL_SYMLINK_PATH}"
            COMMENT "[Symlink Step] Linking development binary directly into system pathways (${FINAL_SYMLINK_PATH})"
        )
    else()
        message(FATAL_ERROR "[Symlink Error] Invalid DEPLOY_STAGE '${ARG_DEPLOY_STAGE}'. Valid options are 'INSTALL' or 'POST_BUILD'.")
    endif()
endfunction()
