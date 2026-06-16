# ==============================================================================
# Module: FindParentDirectory
# Description: Traverses up the directory tree to find a specific directory name.
# ==============================================================================

function(find_parent_directory TARGET_DIR_NAME OUT_RESULT_VAR)
    # Start looking from the directory containing the calling CMakeLists.txt
    set(CURRENT_LOOKUP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(FOUND_DIR "")
    set(LAST_LOOKUP_DIR "")

    # Traverse up the directory tree
    while(NOT "${CURRENT_LOOKUP_DIR}" STREQUAL "${LAST_LOOKUP_DIR}")
        # Extract the name of the current folder
        get_filename_component(CURRENT_DIR_NAME "${CURRENT_LOOKUP_DIR}" NAME)
        
        # Check if the current folder matches the target name
        if("${CURRENT_DIR_NAME}" STREQUAL "${TARGET_DIR_NAME}")
            set(FOUND_DIR "${CURRENT_LOOKUP_DIR}")
            break()
        endif()
        
        # Save current path to detect the filesystem root and prevent infinite loops
        set(LAST_LOOKUP_DIR "${CURRENT_LOOKUP_DIR}")
        # Move up to the parent directory
        get_filename_component(CURRENT_LOOKUP_DIR "${CURRENT_LOOKUP_DIR}" DIRECTORY)
    endwhile()

    # Verify the result and set it to the requested variable in the parent scope
    if(FOUND_DIR)
        set(${OUT_RESULT_VAR} "${FOUND_DIR}" PARENT_SCOPE)
        message(STATUS "Auto-discovered '${TARGET_DIR_NAME}' at: ${FOUND_DIR}")
    else()
        message(FATAL_ERROR "Could not find any directory named '${TARGET_DIR_NAME}' in parent paths!")
    endif()
endfunction()
