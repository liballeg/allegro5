include_directories(${CMAKE_SOURCE_DIR})    # for examples/running.h

add_our_executable(afinfo afinfo.c)
add_our_executable(akaitest WIN32 akaitest.c)
add_our_executable(digitest WIN32 digitest.c)
add_our_executable(filetest WIN32 filetest.c)
add_our_executable(gfxinfo gfxinfo.c)
add_our_executable(mathtest WIN32 mathtest.c)
add_our_executable(miditest WIN32 miditest.c)
add_our_executable(play WIN32 play.c)
add_our_executable(playfli WIN32 playfli.c)
add_our_executable(test WIN32 test.c)
add_our_executable(vesainfo vesainfo.c)

if(WIN32)
    add_our_executable(dibgrab WIN32 win/dibgrab.c)
    add_our_executable(dibhello WIN32 win/dibhello.c)

    if(CMAKE_RC_COMPILER_WORKS)
        add_our_executable(dibsound WIN32 win/dibsound.c win/dibsound.rc)
        add_our_executable(dxwindow WIN32 win/dxwindow.c win/dxwindow.rc)
        add_our_executable(scrsave WIN32 win/scrsave.c win/scrsave.rc)
        # I don't know if there is a better way.
        add_custom_command(
            TARGET scrsave
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${CMAKE_CURRENT_BINARY_DIR}/scrsave.exe"
                    "${CMAKE_CURRENT_BINARY_DIR}/scrsave.scr"
            )
    endif()
endif(WIN32)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
