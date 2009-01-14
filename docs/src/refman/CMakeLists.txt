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
    system
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
set(PROTOS_TIMESTAMP ${PROTOS}.timestamp)

set(HTML_REFS ${CMAKE_CURRENT_BINARY_DIR}/html_refs)
set(HTML_REFS_TIMESTAMP ${HTML_REFS}.timestamp)
set(DUMMY_REFS ${CMAKE_CURRENT_BINARY_DIR}/dummy_refs)
set(DUMMY_REFS_TIMESTAMP ${DUMMY_REFS}.timestamp)

set(SCRIPT_DIR ${CMAKE_SOURCE_DIR}/docs/scripts)
set(MAKE_PROTOS ${SH} ${SCRIPT_DIR}/make_protos)
set(MAKE_HTML_REFS ${SH} ${SCRIPT_DIR}/make_html_refs)
set(MAKE_DUMMY_REFS ${SH} ${SCRIPT_DIR}/make_dummy_refs)
set(MAKE_PAGE ${SH} ${SCRIPT_DIR}/make_page --protos ${PROTOS})
set(MAKE_MAN ${SH} ${SCRIPT_DIR}/make_man --protos ${PROTOS})

#-----------------------------------------------------------------------------#
#
#   Protos
#
#-----------------------------------------------------------------------------#

# The protos file is a list of function prototypes and type declarations
# which can then be embedded into the documentation.
# Rebuilding the documentation whenever a source file changes is irritating,
# especially as public prototypes rarely change.  Thus we keep a second file
# called protos.timestamp which reflects the last time that the protos file
# changed.  We declare _that_ file as the dependency of other targets.

# XXX for some reason the html target sometimes won't build directly after
# protos.timestamp changes.  Running make a second time works, however.

file(GLOB_RECURSE ALL_SRCS
    ${CMAKE_SOURCE_DIR}/src/*.[ch]
    ${CMAKE_SOURCE_DIR}/src/*.[ch]pp
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.inl
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]pp
    )

add_custom_command(
    OUTPUT ${PROTOS} ${PROTOS_TIMESTAMP}
    DEPENDS ${ALL_SRCS}
    COMMAND ${MAKE_PROTOS} --output ${PROTOS} ${ALL_SRCS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROTOS} ${PROTOS_TIMESTAMP}
    )

# For testing.
add_custom_target(gen_protos DEPENDS ${PROTOS})

#-----------------------------------------------------------------------------#
#
#   HTML
#
#-----------------------------------------------------------------------------#

# The html_refs file contains link definitions for each API entry.
# It's used to resolve references across HTML pages.

add_custom_command(
    OUTPUT ${HTML_REFS} ${HTML_REFS_TIMESTAMP}
    DEPENDS ${PAGES_TXT}
    COMMAND ${MAKE_HTML_REFS} --output ${HTML_REFS} ${PAGES_TXT}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${HTML_REFS} ${HTML_REFS_TIMESTAMP}
    )

if(WANT_DOCS_HTML)
    set(HTML_PAGES)
    foreach(page ${PAGES})
        add_custom_command(
            OUTPUT ${page}.html
            DEPENDS ${PROTOS_TIMESTAMP} ${HTML_REFS_TIMESTAMP} ${page}.txt
            COMMAND
                ${MAKE_PAGE} ${HTML_REFS}
                ${CMAKE_CURRENT_SOURCE_DIR}/${page}.txt
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
endif(WANT_DOCS_HTML)

#-----------------------------------------------------------------------------#
#
#   Man pages
#
#-----------------------------------------------------------------------------#

set(MANDIR "man" CACHE STRING "Install man pages into this directory")

if(WANT_DOCS_MAN)
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
            DEPENDS ${PROTOS_TIMESTAMP} ${page}
            COMMAND ${MAKE_MAN} ${page}
            WORKING_DIRECTORY ${MAN_DIR}
            )

        list(APPEND MAN_PAGES ${outputs})
    endforeach(page)

    add_custom_target(man ALL DEPENDS ${MAN_PAGES})

    install(FILES ${MAN_PAGES}
            DESTINATION ${MANDIR}/man3
            )
endif(WANT_DOCS_MAN)

#-----------------------------------------------------------------------------#
#
#   Info
#
#-----------------------------------------------------------------------------#

add_custom_command(
    OUTPUT ${DUMMY_REFS} ${DUMMY_REFS_TIMESTAMP}
    DEPENDS ${PAGES_TXT}
    COMMAND ${MAKE_DUMMY_REFS} --output ${DUMMY_REFS} ${PAGES_TXT}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DUMMY_REFS} ${DUMMY_REFS_TIMESTAMP}
    )

add_custom_target(gen_dummy_refs DEPENDS ${DUMMY_REFS})

if(WANT_DOCS_INFO AND MAKEINFO)
    add_custom_target(info ALL DEPENDS refman.info)
    add_custom_command(
        OUTPUT refman.info
        DEPENDS refman.texi
        COMMAND ${MAKEINFO} --paragraph-indent 0 refman.texi
        )
    add_custom_command(
        OUTPUT refman.texi
        DEPENDS ${PROTOS_TIMESTAMP} ${DUMMY_REFS_TIMESTAMP} ${PAGES_TXT}
        COMMAND ${MAKE_PAGE} --postprocess texinfo
                ${DUMMY_REFS} ${PAGES_TXT}
                -o refman.texi
                --standalone
        )
endif(WANT_DOCS_INFO AND MAKEINFO)

#-----------------------------------------------------------------------------#
#
#   LaTeX (PDF)
#
#-----------------------------------------------------------------------------#

if(WANT_DOCS_PDF AND PDFLATEX_COMPILER)
    set(EXTRA_LATEX_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/header.tex)

    add_custom_target(latex DEPENDS refman.tex)
    add_custom_command(
        OUTPUT refman.tex
        DEPENDS ${PROTOS_TIMESTAMP} ${DUMMY_REFS_TIMESTAMP} ${PAGES_TXT}
            ${EXTRA_LATEX_HEADER}
        COMMAND ${MAKE_PAGE} --postprocess latex
            ${DUMMY_REFS} ${PAGES_TXT}
            --include-in-header ${EXTRA_LATEX_HEADER}
            -o refman.tex
            --standalone --number-sections
        )

    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
        "refman.aux;refman.aux.bak;refman.log;refman.out;refman.toc")

    add_custom_target(pdf ALL DEPENDS refman.pdf)
    add_custom_command(
        OUTPUT refman.pdf
        DEPENDS refman.tex
        # Repeat three times to get cross references correct.
        COMMAND ${PDFLATEX_COMPILER} refman.tex
        COMMAND ${PDFLATEX_COMPILER} refman.tex
        COMMAND ${PDFLATEX_COMPILER} refman.tex
        )
endif(WANT_DOCS_PDF AND PDFLATEX_COMPILER)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
