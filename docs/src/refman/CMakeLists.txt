set(PAGES
    color
    config
    display
    events
    font
    fshook
    graphics
    iio
    joystick
    kcm_audio
    keyboard
    memory
    mouse
    opengl
    path
    state
    threads
    time
    timer
    )

set(PAGES_TXT)
foreach(page ${PAGES})
    list(APPEND PAGES_TXT ${CMAKE_CURRENT_SOURCE_DIR}/${page}.txt)
endforeach(page)

#-----------------------------------------------------------------------------#
#
#   Paths
#
#-----------------------------------------------------------------------------#

set(PROTOS ${CMAKE_CURRENT_BINARY_DIR}/protos)
set(SCRIPT_DIR ${CMAKE_SOURCE_DIR}/docs/scripts)
set(MAKE_PROTOS ${SH} ${SCRIPT_DIR}/make_protos)
set(MAKE_PAGE ${SH} ${SCRIPT_DIR}/make_page --protos ${PROTOS})
set(MAKE_MAN ${SH} ${SCRIPT_DIR}/make_man --protos ${PROTOS})

#-----------------------------------------------------------------------------#
#
#   Protos
#
#-----------------------------------------------------------------------------#

# The protos file is a list of function prototypes and type declarations
# which can then be embedded into the documentation.

file(GLOB_RECURSE ALL_SRCS
    ${CMAKE_SOURCE_DIR}/src/*.[ch]
    ${CMAKE_SOURCE_DIR}/src/*.[ch]pp
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.inl
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]pp
    )

add_custom_command(
    OUTPUT protos
    DEPENDS ${ALL_SRCS}
    COMMAND ${MAKE_PROTOS} ${ALL_SRCS} > ${PROTOS}
    )

# For testing.
add_custom_target(gen_protos DEPENDS protos)

#-----------------------------------------------------------------------------#
#
#   HTML
#
#-----------------------------------------------------------------------------#

if(WANT_HTML)
    set(HTML_PAGES)
    foreach(page ${PAGES})
        add_custom_command(
            OUTPUT ${page}.html
            DEPENDS ${PROTOS} ${page}.txt
            COMMAND
                ${MAKE_PAGE} ${CMAKE_CURRENT_SOURCE_DIR}/${page}.txt
                -o ${page}.html
                -c pandoc.css
                --standalone --toc
            )
        list(APPEND HTML_PAGES ${page}.html)
    endforeach(page)
    add_custom_target(html ALL DEPENDS ${HTML_PAGES})

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/../pandoc.css
        ${CMAKE_CURRENT_BINARY_DIR}/pandoc.css
        COPY_ONLY)
endif(WANT_HTML)

#-----------------------------------------------------------------------------#
#
#   Man pages
#
#-----------------------------------------------------------------------------#

set(MANDIR "man" CACHE STRING "Install man pages into this directory")

if(WANT_MAN)
    set(MAN_DIR ${CMAKE_CURRENT_BINARY_DIR}/man)
    make_directory(${MAN_DIR})

    set(MAN_PAGES)
    foreach(page ${PAGES_TXT})
        # Figure out the man pages that would be generated from this file.
        file(STRINGS ${page} lines REGEX "# API: ")
        string(REGEX REPLACE "[#]* API: " ";" entries ${lines})

        set(outputs)
        foreach(entry ${entries})
            list(APPEND outputs ${CMAKE_CURRENT_BINARY_DIR}/man/${entry}.3)
        endforeach(entry)

        add_custom_command(
            OUTPUT ${outputs}
            DEPENDS ${PROTOS} ${page}
            COMMAND ${MAKE_MAN} ${page}
            WORKING_DIRECTORY ${MAN_DIR}
            )

        list(APPEND MAN_PAGES ${outputs})
    endforeach(page)

    add_custom_target(man ALL DEPENDS ${MAN_PAGES})

    install(FILES ${MAN_PAGES}
            DESTINATION ${MANDIR}
            )
endif(WANT_MAN)

#-----------------------------------------------------------------------------#
#
#   Info
#
#-----------------------------------------------------------------------------#

if(WANT_INFO AND MAKEINFO)
    add_custom_target(info ALL DEPENDS refman.info)
    add_custom_command(
        OUTPUT refman.info
        DEPENDS refman.texi
        COMMAND ${MAKEINFO} --paragraph-indent 0 refman.texi
        )
    add_custom_command(
        OUTPUT refman.texi
        DEPENDS ${PROTOS} ${PAGES_TXT}
        COMMAND ${MAKE_PAGE} ${PAGES_TXT} -o refman.texi --standalone
        )
endif(WANT_INFO AND MAKEINFO)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
