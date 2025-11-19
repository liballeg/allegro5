# Delay-load helper for Windows linkers.
#
# Brief: Receives the name of a CMake variable that holds one or more library
#   names, and makes the corresponding DLLs to be delay-loaded on Windows.
#
# Details:
#   - The argument is the variable's name (not its value). The variable should
#     contain a single library or a list of libraries whose DLLs should be
#     delay-loaded.
#   - For each library listed, the function adds the appropriate linker options
#     to request delay-loading of the matching DLL and ensures the delay-load
#     support library is linked when required.
#   - On non-Windows platforms, this function performs no action.
#
# Notes:
#   - The delay-load mechanism (aka lazy loading) defers loading of the
#     specified DLLs until the first time a symbol from each DLL is called at
#     runtime.
function(DelayLoad libvar)
    if(WANT_DELAYLOAD AND WIN32)
        set(OUT "")
        foreach(lib ${${libvar}})
            if(MSVC)
                execute_process(
                    COMMAND powershell -c "${PROJECT_SOURCE_DIR}/cmake/implib2dll.ps1 '${lib}'"
                    OUTPUT_VARIABLE DLL
                    ERROR_VARIABLE ERR
                    RESULT_VARIABLE RES
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                # TODO: /DELAYLOAD:${DLL} should go to target_link_options
                # use delayimp.lib only once
                # cache all that somehow
                list(APPEND OUT "${lib}" delayimp.lib "-DELAYLOAD:${DLL}")
            else(MSVC)
                execute_process(
                    COMMAND sh -c "${PROJECT_SOURCE_DIR}/cmake/implib_delay.sh '${lib}'"
                    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/delayload"
                    OUTPUT_VARIABLE DELAYED_LIB
                    ERROR_VARIABLE ERR
                    RESULT_VARIABLE RES
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                list(APPEND OUT "${PROJECT_BINARY_DIR}/delayload/${DELAYED_LIB}")
            endif(MSVC)
            if (RES)
                message(FATAL_ERROR "DelayLoad: Failed to process library '${lib}': ${ERR}")
            endif(RES)
        endforeach()
        set(${libvar} ${OUT} PARENT_SCOPE)
    endif(WANT_DELAYLOAD AND WIN32)
endfunction(DelayLoad)

if(WANT_DELAYLOAD AND WIN32 AND NOT MSVC)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/delayload")
endif(WANT_DELAYLOAD AND WIN32 AND NOT MSVC)
