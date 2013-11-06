# Conditionally build an example program.  If any of its arguments is the exact
# string "x", do nothing.  Otherwise strip off the "x" prefixes on arguments
# and link to that target.
function(example name)
    set(is_console)
    set(sources)
    set(libs)
    set(android_stl)

    foreach(arg ${ARGN})
        if(arg STREQUAL "x")
            message(STATUS "Not building ${name}")
            return()
        endif()
        if(arg STREQUAL "CONSOLE")
            set(is_console ON)
        elseif(arg MATCHES "[.]c$")
            list(APPEND sources ${arg})
        elseif(arg MATCHES "[.]cpp$")
            list(APPEND sources ${arg})
            set(android_stl stlport_shared)
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

    list(REMOVE_DUPLICATES libs)

    if(WIN32)
        if(is_console)
            # We need stdout and stderr available from cmd.exe,
            # so we must not use WIN32 here.
            set(EXECUTABLE_TYPE)
        else()
            set(EXECUTABLE_TYPE "WIN32")
        endif(is_console)
    endif(WIN32)

    if(IPHONE)
        set(EXECUTABLE_TYPE MACOSX_BUNDLE)
    endif(IPHONE)

    if(ANDROID)
        if(is_console)
            message(STATUS "Not building ${name} - console program")
            return()
        endif()
        add_android_app("${name}" "${sources}" "${libs}" "${stl}")
    else()
        add_our_executable("${name}" "${sources}" "${libs}")
    endif()

endfunction(example)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
