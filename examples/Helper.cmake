# Conditionally build an example program.  If any of its arguments is the exact
# string "x", do nothing.  Otherwise strip off the "x" prefixes on arguments
# and link to that target.
function(example name)
    # Use cmake_parse_arguments first.
    set(flags CONSOLE)
    set(single_args) # none
    set(accum_args DATA EXTRA_LIBS)
    cmake_parse_arguments(MYOPTS "${flags}" "${single_args}" "${accum_args}"
        ${ARGN})

    # Parse what remains of the argument list manually.
    set(sources)
    set(libs)
    foreach(arg ${MYOPTS_UNPARSED_ARGUMENTS})
        if(arg STREQUAL "x")
            message(STATUS "Not building ${name}")
            return()
        endif()
        if(arg MATCHES "[.]c$")
            list(APPEND sources ${arg})
        elseif(arg MATCHES "[.]cpp$")
            list(APPEND sources ${arg})
        else()
            string(REGEX REPLACE "^x" "" arg ${arg})
            list(APPEND libs ${arg})
        endif()
    endforeach()

    # If no sources are listed assume a single C source file.
    if(NOT sources)
        set(sources "${name}.c")
    endif()

    # Prepend the base libraries.
    if(ANDROID)
        set(libs ${ALLEGRO_LINK_WITH} ${libs})
    else()
        set(libs ${ALLEGRO_LINK_WITH} ${ALLEGRO_MAIN_LINK_WITH} ${libs})
    endif()

    # Popup error messages.
    if(WANT_POPUP_EXAMPLES AND SUPPORT_NATIVE_DIALOG)
        list(APPEND libs ${NATIVE_DIALOG_LINK_WITH})
    endif()

    # Monolith contains all other libraries which were enabled.
    if(WANT_MONOLITH)
        set(libs ${ALLEGRO_MONOLITH_LINK_WITH})
    endif()

    # Append the extra, non-Allegro libraries.
    foreach(lib ${MYOPTS_EXTRA_LIBS})
        list(APPEND libs ${lib})
    endforeach()

    list(REMOVE_DUPLICATES libs)

    if(WIN32)
        if(MYOPTS_CONSOLE)
            # We need stdout and stderr available from cmd.exe,
            # so we must not use WIN32 here.
            set(EXECUTABLE_TYPE)
        else()
            set(EXECUTABLE_TYPE "WIN32")
        endif()
    endif(WIN32)

    if(IPHONE)
        set(EXECUTABLE_TYPE MACOSX_BUNDLE)
    endif(IPHONE)

    if(ANDROID)
        if(MYOPTS_CONSOLE)
            message(STATUS "Not building ${name} - console program")
            return()
        endif()
        add_copy_commands(
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "${CMAKE_CURRENT_BINARY_DIR}/${name}.project/app/src/main/assets/data"
            assets
            "${MYOPTS_DATA}"
            )
        add_android_app("${name}" "${sources};${assets}")
    elseif(IPHONE)
        add_our_executable("${name}" SRCS "${sources};${CMAKE_CURRENT_SOURCE_DIR}/data"
                           LIBS "${libs}")
        set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/data" PROPERTIES
                                    MACOSX_PACKAGE_LOCATION "Resources")
    else()
        add_our_executable("${name}" SRCS "${sources}" LIBS "${libs}")
    endif()
    add_dependencies("${name}" copy_example_data)
endfunction(example)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
