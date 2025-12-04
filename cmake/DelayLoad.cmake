# TODO: move from addons/acodec/CMakeLists.txt
# as it is not available yet while building audio.
function(get_dll_name implib dllname_var)
    if(MINGW)
        # Guess the name of dlltool from gcc.
        string(REGEX REPLACE "gcc.*" dlltool DLLTOOL ${CMAKE_C_COMPILER})
        execute_process(
            COMMAND ${DLLTOOL} -I ${implib}
            OUTPUT_VARIABLE dllname
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    elseif(MSVC)
        # Not sure this is the best way.
        execute_process(
            COMMAND lib /LIST ${implib}
            OUTPUT_VARIABLE output
            )
        if(output STREQUAL "")
            message("WARNING: Failed to execute lib /list")
        else()
            string(REGEX MATCH "[^\n]+[.]dll" dllname "${output}")
        endif()
    endif()
    if(NOT dllname)
        # Guess from the basename.
        get_filename_component(basename "${implib}" NAME_WE)
        set(dllname "${basename}.dll")
    endif()
    message(STATUS "DLL name for ${implib}: ${dllname}")
    set(${dllname_var} ${dllname} PARENT_SCOPE)
endfunction(get_dll_name)

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
function(DelayLoad libvar linkvar)
    if(WANT_DELAYLOAD AND WIN32)
        set(OUT "")
        set(OPTS "")
        foreach(lib ${${libvar}})
            if(lib MATCHES "^[$].*")  # Skip generator expressions
                list(APPEND OUT "${lib}")
                continue()
            endif()
            if(MSVC)
                get_dll_name("${lib}" DLL)
                list(APPEND OUT "${lib}")
                if(NOT ${linkvar})
                    list(APPEND OUT "delayimp.lib")
                endif()
                list(APPEND OPTS "/DELAYLOAD:${DLL}")
                set(${linkvar} ${OPTS} PARENT_SCOPE)
            else(MSVC)
                execute_process(
                    COMMAND sh -c "${PROJECT_SOURCE_DIR}/cmake/implib_delay.sh '${lib}'"
                    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/delayload"
                    OUTPUT_VARIABLE DELAYED_LIB
                    ERROR_VARIABLE ERR
                    RESULT_VARIABLE RES
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                list(APPEND OUT "${DELAYED_LIB}")
            endif(MSVC)
            if (RES)
                message(FATAL_ERROR "DelayLoad: Failed to process library '${lib}': ${ERR}")
            endif(RES)
        endforeach()
        set(${libvar} ${OUT} PARENT_SCOPE)
    endif(WANT_DELAYLOAD AND WIN32)
endfunction(DelayLoad)

if(WANT_DELAYLOAD AND WIN32)
    if(MSVC)
        set(DELAYLOAD_LINKER_OPTIONS "delayimp.lib")
        if(NOT DEFINED ENV{WindowsSdkDir})
            # This should not happen if ran from a proper Visual Studio environment
            # This is mostly a fallback for CI
            block(SCOPE_FOR VARIABLES)
                cmake_host_system_information(RESULT root QUERY WINDOWS_REGISTRY
                    "HKLM\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots"
                    VALUE "KitsRoot10"
                )
                cmake_host_system_information(RESULT sdks QUERY WINDOWS_REGISTRY
                    "HKLM\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots"
                    SUBKEYS
                )
                list(POP_BACK sdks ver)
                set(ENV{WindowsSDKDir} ${root})
                set(ENV{WindowsSDKVersion} ${ver})
                message(STATUS "DelayLoad: inferred Windows SDK path: $ENV{WindowsSDKDir}, version: $ENV{WindowsSDKVersion}")
            endblock()
        endif()
    else(MSVC)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/delayload")
        link_directories("${PROJECT_BINARY_DIR}/delayload")
    endif(MSVC)
endif(WANT_DELAYLOAD AND WIN32)