function(cmake_apply_patches)
    cmake_parse_arguments(_ap "QUIET" "SOURCE_PATH" "PATCHES" ${ARGN})

	find_package(Git REQUIRED)
    set(PATCHNUM 0)
    foreach(PATCH ${_ap_PATCHES})
        get_filename_component(ABSOLUTE_PATCH "${PATCH}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")
        set(LOGNAME "patch-${PATCHNUM}")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} apply "${ABSOLUTE_PATCH}" --ignore-whitespace --whitespace=nowarn --verbose
            OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${LOGNAME}-out.log
            ERROR_FILE ${CMAKE_CURRENT_BINARY_DIR}/${LOGNAME}-err.log
            WORKING_DIRECTORY ${_ap_SOURCE_PATH}
            RESULT_VARIABLE error_code
        )

        if(error_code AND NOT _ap_QUIET)
            message(STATUS "Applying patch failed. This is expected if this patch was previously applied.")
        endif()

        math(EXPR PATCHNUM "${PATCHNUM}+1")
    endforeach()
endfunction()
