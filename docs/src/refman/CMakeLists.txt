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
    primitives
    state
    system
    threads
    time
    timer
    utf8
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
set(MAKE_HTML_REFS ${AWK} -f ${SCRIPT_DIR}/make_html_refs)
set(MAKE_DUMMY_REFS ${AWK} -f ${SCRIPT_DIR}/make_dummy_refs)
set(MAKE_PAGE ${SH} ${SCRIPT_DIR}/make_page --protos ${PROTOS})
set(MAKE_MAN ${SH} ${SCRIPT_DIR}/make_man --protos ${PROTOS})
set(INSERT_TIMESTAMP ${SH} ${SCRIPT_DIR}/insert_timestamp)
set(MAKE_SEARCH_INDEX ${PYTHON} ${SCRIPT_DIR}/make_search_index.py)

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

file(GLOB_RECURSE ALL_SRCS
    ${CMAKE_SOURCE_DIR}/src/*.[ch]
    ${CMAKE_SOURCE_DIR}/src/*.[ch]pp
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.inl
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]pp
    )

add_custom_command(
    # For some reason the timestamp file must be listed first, otherwise make
    # doesn't notice that it's updated until you run make a second time.
    OUTPUT ${PROTOS_TIMESTAMP} ${PROTOS}
    DEPENDS ${ALL_SRCS}
    COMMAND ${MAKE_PROTOS} ${ALL_SRCS} > ${PROTOS}
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
    # As above, the timestamp file must be listed first.
    OUTPUT ${HTML_REFS_TIMESTAMP} ${HTML_REFS}
    DEPENDS ${PAGES_TXT}
    COMMAND ${MAKE_HTML_REFS} ${PAGES_TXT} > ${HTML_REFS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${HTML_REFS} ${HTML_REFS_TIMESTAMP}
    )

if(PYTHON)
    set(SEARCH_INDEX_JS search_index.js)
    add_custom_command(
        DEPENDS ${HTML_REFS}
        OUTPUT search_index.js
        COMMAND ${MAKE_SEARCH_INDEX}
        )
else()
    set(SEARCH_INDEX_JS)
endif()

if(WANT_DOCS_HTML)
    foreach(inc inc.a inc.z)
        add_custom_command(
            OUTPUT ${inc}.html
            DEPENDS ${inc}.txt
            COMMAND ${PANDOC} ${CMAKE_CURRENT_SOURCE_DIR}/${inc}.txt
                    -o ${inc}.html
            )
    endforeach(inc)

    set(HTML_PAGES)
    foreach(page ${PAGES} index)
        add_custom_command(
            OUTPUT ${page}.html
            DEPENDS ${PROTOS_TIMESTAMP} ${HTML_REFS_TIMESTAMP} ${page}.txt
                ${CMAKE_CURRENT_BINARY_DIR}/inc.a.html
                ${CMAKE_CURRENT_BINARY_DIR}/inc.z.html
                ${SEARCH_INDEX_JS}
            COMMAND
                ${INSERT_TIMESTAMP} inc.timestamp.html
            COMMAND
                ${MAKE_PAGE}
                ${CMAKE_CURRENT_SOURCE_DIR}/${page}.txt
                ${HTML_REFS}
                -o ${page}.html
                --include-before-body inc.a.html
                --include-after-body inc.z.html
                --include-after-body inc.timestamp.html
                -c pandoc.css
                -C custom_header.html
                --standalone --toc
            )
        list(APPEND HTML_PAGES ${CMAKE_CURRENT_BINARY_DIR}/${page}.html)
    endforeach(page)
    add_custom_target(html ALL DEPENDS ${HTML_PAGES})

    foreach(file pandoc.css custom_header.html autosuggest.js)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/../${file}
            ${CMAKE_CURRENT_BINARY_DIR}/${file}
            COPY_ONLY)
    endforeach(file)
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
    # As above, the timestamp file must be listed first.
    OUTPUT ${DUMMY_REFS_TIMESTAMP} ${DUMMY_REFS}
    DEPENDS ${PAGES_TXT}
    COMMAND ${MAKE_DUMMY_REFS} ${PAGES_TXT} > ${DUMMY_REFS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DUMMY_REFS} ${DUMMY_REFS_TIMESTAMP}
    )

add_custom_target(gen_dummy_refs DEPENDS ${DUMMY_REFS})

if(WANT_DOCS_INFO AND PANDOC_WITH_TEXINFO AND MAKEINFO)
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
endif(WANT_DOCS_INFO AND PANDOC_WITH_TEXINFO AND MAKEINFO)

#-----------------------------------------------------------------------------#
#
#   LaTeX (PDF)
#
#-----------------------------------------------------------------------------#

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

if(WANT_DOCS_PDF AND PDFLATEX_COMPILER)
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
#
#   Tags file
#
#-----------------------------------------------------------------------------#

if(CTAGS)
    add_custom_target(gen_tags DEPENDS tags)
    add_custom_command(
        OUTPUT tags
        DEPENDS ${PAGES_TXT}
        COMMAND ${CTAGS}
            --langdef=allegrodoc
            --langmap=allegrodoc:.txt
            "--regex-allegrodoc=/^#+ API: (.+)/\\1/"
            ${PAGES_TXT}
        VERBATIM
        )
endif(CTAGS)

#-----------------------------------------------------------------------------#
#
#   Consistency check
#
#-----------------------------------------------------------------------------#

add_custom_target(check_consistency
    DEPENDS ${PROTOS}
    COMMAND ${SH} ${SCRIPT_DIR}/check_consistency --protos ${PROTOS}
            ${PAGES_TXT}
    )

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
