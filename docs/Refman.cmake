set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_REFMAN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/refman)

# Put these in the order that they should appear in the Info or PDF manual.
set(PAGES
    getting_started

    config
    display
    events
    file
    fixed
    fshook
    fullscreen_mode
    graphics
    joystick
    keyboard
    memory
    misc
    monitor
    mouse
    path
    state
    system
    threads
    time
    timer
    transformations
    utf8

    platform
    direct3d
    opengl

    audio
    acodec
    color
    font
    image
    main
    memfile
    native_dialog
    physfs
    primitives
    )

set(PAGES_TXT)
foreach(page ${PAGES})
    list(APPEND PAGES_TXT ${SRC_REFMAN_DIR}/${page}.txt)
endforeach(page)

set(IMAGES
    primitives1
    primitives2
    )


#-----------------------------------------------------------------------------#
#
#   Paths
#
#-----------------------------------------------------------------------------#

set(HTML_DIR ${CMAKE_CURRENT_BINARY_DIR}/html/refman)
set(MAN_DIR ${CMAKE_CURRENT_BINARY_DIR}/man)
set(INFO_DIR ${CMAKE_CURRENT_BINARY_DIR}/info)
set(TEXI_DIR ${CMAKE_CURRENT_BINARY_DIR}/texi)
set(LATEX_DIR ${CMAKE_CURRENT_BINARY_DIR}/latex)
set(PDF_DIR ${CMAKE_CURRENT_BINARY_DIR}/pdf)

set(PROTOS ${CMAKE_CURRENT_BINARY_DIR}/protos)
set(PROTOS_TIMESTAMP ${PROTOS}.timestamp)
set(HTML_REFS ${CMAKE_CURRENT_BINARY_DIR}/html_refs)
set(HTML_REFS_TIMESTAMP ${HTML_REFS}.timestamp)
set(DUMMY_REFS ${CMAKE_CURRENT_BINARY_DIR}/dummy_refs)
set(DUMMY_REFS_TIMESTAMP ${DUMMY_REFS}.timestamp)
set(INDEX_ALL ${CMAKE_CURRENT_BINARY_DIR}/index_all.txt)
set(SEARCH_INDEX_JS ${HTML_DIR}/search_index.js)

set(SCRIPT_DIR ${CMAKE_SOURCE_DIR}/docs/scripts)
set(MAKE_PROTOS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/make_protos)
set(MAKE_HTML_REFS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/make_html_refs)
set(MAKE_INDEX ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/make_index)
set(MAKE_DUMMY_REFS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/make_dummy_refs)
set(MAKE_DOC ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/make_doc --protos ${PROTOS})
set(INSERT_TIMESTAMP ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/insert_timestamp)
set(MAKE_SEARCH_INDEX ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/make_search_index)

set(DAWK_SOURCES scripts/aatree.c scripts/dawk.c scripts/trex.c)

add_executable(make_protos scripts/make_protos.c ${DAWK_SOURCES})
add_executable(make_html_refs scripts/make_html_refs.c ${DAWK_SOURCES})
add_executable(make_dummy_refs scripts/make_dummy_refs.c ${DAWK_SOURCES})
add_executable(make_index scripts/make_index.c ${DAWK_SOURCES})
add_executable(make_doc
    scripts/make_doc.c
    scripts/make_man.c
    scripts/make_single.c
    ${DAWK_SOURCES})
add_executable(insert_timestamp
   scripts/insert_timestamp.c
   ${DAWK_SOURCES})
add_executable(make_search_index scripts/make_search_index.c ${DAWK_SOURCES})

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

# We can get into a situation where the protos file is newer than the source
# files (up-to-date) but the protos.timestamp is older than the source files.
# If the protos and protos.timestamp files are identical then each time
# you run make, it will compare them and find them equal, so protos.timestamp
# won't be updated.  However that check is instantaneous.

# ALL_SRCS is split into multiple lists, otherwise the make_protos command
# line is too long for Windows >:-( We use relative paths for the same reason.
file(GLOB_RECURSE ALL_SRCS1
    ${CMAKE_SOURCE_DIR}/src/*.[chm]
    ${CMAKE_SOURCE_DIR}/src/*.[ch]pp
    )
file(GLOB_RECURSE ALL_SRCS2
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.inl
    )
file(GLOB_RECURSE ALL_SRCS3
    ${CMAKE_SOURCE_DIR}/addons/*.[chm]
    ${CMAKE_SOURCE_DIR}/addons/*.[ch]pp
    )

foreach(x ${ALL_SRCS1})
    file(RELATIVE_PATH xrel ${CMAKE_SOURCE_DIR} ${x})
    list(APPEND ALL_SRCS1_REL ${xrel})
endforeach()
foreach(x ${ALL_SRCS2})
    file(RELATIVE_PATH xrel ${CMAKE_SOURCE_DIR} ${x})
    list(APPEND ALL_SRCS2_REL ${xrel})
endforeach()
foreach(x ${ALL_SRCS3})
    file(RELATIVE_PATH xrel ${CMAKE_SOURCE_DIR} ${x})
    list(APPEND ALL_SRCS3_REL ${xrel})
endforeach()

add_custom_command(
    OUTPUT ${PROTOS}
    DEPENDS ${ALL_SRCS1} ${ALL_SRCS2} ${ALL_SRCS3} make_protos
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${MAKE_PROTOS} ${ALL_SRCS1_REL} > ${PROTOS}
    COMMAND ${MAKE_PROTOS} ${ALL_SRCS2_REL} >> ${PROTOS}
    COMMAND ${MAKE_PROTOS} ${ALL_SRCS3_REL} >> ${PROTOS}
    )

add_custom_command(
    OUTPUT ${PROTOS_TIMESTAMP}
    DEPENDS ${PROTOS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROTOS} ${PROTOS_TIMESTAMP}
    )

# For testing (command line too long for Windows)
if(NOT WIN32)
    add_custom_target(gen_protos DEPENDS ${PROTOS})
endif()

#-----------------------------------------------------------------------------#
#
#   HTML
#
#-----------------------------------------------------------------------------#

# The html_refs file contains link definitions for each API entry.
# It's used to resolve references across HTML pages.
# The search_index.js file contains definitions for the autosuggest widget.

if(PANDOC_STRIP_UNDERSCORES)
    set(STRIP_UNDERSCORES "--strip-underscores")
else()
    set(STRIP_UNDERSCORES "")
endif()

add_custom_command(
    OUTPUT ${HTML_REFS}
    DEPENDS ${PAGES_TXT} make_html_refs
    COMMAND ${MAKE_HTML_REFS} ${STRIP_UNDERSCORES} ${PAGES_TXT} > ${HTML_REFS}
    )

add_custom_command(
    OUTPUT ${HTML_REFS_TIMESTAMP}
    DEPENDS ${HTML_REFS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${HTML_REFS} ${HTML_REFS_TIMESTAMP}
    )

add_custom_command(
    OUTPUT ${INDEX_ALL}
    DEPENDS ${HTML_REFS_TIMESTAMP} make_index
    COMMAND ${MAKE_INDEX} ${HTML_REFS} > ${INDEX_ALL}
    )

add_custom_command(
    OUTPUT ${SEARCH_INDEX_JS}
    DEPENDS ${HTML_REFS_TIMESTAMP} make_search_index
    COMMAND ${MAKE_SEARCH_INDEX} ${HTML_REFS} > ${SEARCH_INDEX_JS}
    )

if(WANT_DOCS_HTML)
    foreach(inc inc.a inc.z)
        # Use native Windows syntax to avoid "c:/foo.txt" being treated as a
        # remote URI by Pandoc 1.5 and 1.6.
        file(TO_NATIVE_PATH ${SRC_REFMAN_DIR}/${inc}.txt pandoc_src)
        add_custom_command(
            OUTPUT ${inc}.html
            DEPENDS ${SRC_REFMAN_DIR}/${inc}.txt
            COMMAND ${PANDOC} ${pandoc_src} -o ${inc}.html
            )
    endforeach(inc)

    set(HTML_PAGES)
    foreach(page ${PAGES} index index_all)
        if(page STREQUAL "index_all")
            set(page_src ${INDEX_ALL})
        else()
            set(page_src ${SRC_REFMAN_DIR}/${page}.txt)
        endif()
        add_custom_command(
            OUTPUT ${HTML_DIR}/${page}.html
            DEPENDS
                ${PROTOS_TIMESTAMP}
                ${HTML_REFS_TIMESTAMP}
                ${page_src}
                ${CMAKE_CURRENT_BINARY_DIR}/inc.a.html
                ${CMAKE_CURRENT_BINARY_DIR}/inc.z.html
                ${SEARCH_INDEX_JS}
                make_doc
                insert_timestamp
            COMMAND
                ${INSERT_TIMESTAMP} ${CMAKE_SOURCE_DIR}/include/allegro5/base.h > inc.timestamp.html
            COMMAND
                ${MAKE_DOC}
                --to html
                --raise-sections
                --include-before-body inc.a.html
                --include-after-body inc.timestamp.html
                --include-after-body inc.z.html
                --css pandoc.css
                --include-in-header ${SRC_DIR}/custom_header.html
                --standalone
                --toc
                -- ${page_src} ${HTML_REFS}
                > ${HTML_DIR}/${page}.html
            )
        list(APPEND HTML_PAGES ${HTML_DIR}/${page}.html)
    endforeach(page)

    set(HTML_IMAGES)
    foreach(image ${IMAGES})
        add_custom_command(
            OUTPUT ${HTML_DIR}/images/${image}.png
            DEPENDS
                ${SRC_REFMAN_DIR}/images/${image}.png
            COMMAND 
                "${CMAKE_COMMAND}" -E copy
                "${SRC_REFMAN_DIR}/images/${image}.png" "${HTML_DIR}/images/${image}.png"
            ) 
         list(APPEND HTML_IMAGES ${HTML_DIR}/images/${image}.png)
    endforeach(image)
    
    add_custom_target(html ALL DEPENDS ${HTML_PAGES} ${HTML_IMAGES})

    foreach(file pandoc.css autosuggest.js)
        configure_file(
            ${SRC_DIR}/${file}
            ${HTML_DIR}/${file}
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
    make_directory(${MAN_DIR})

    set(MAN_PAGES)
    foreach(page ${PAGES_TXT})
        # Figure out the man pages that would be generated from this file.
        file(STRINGS ${page} lines REGEX "# API: ")
        if(lines)
            string(REGEX REPLACE "[#]* API: " ";" entries ${lines})

            set(outputs)
            foreach(entry ${entries})
                list(APPEND outputs ${MAN_DIR}/${entry}.3)
            endforeach(entry)

            add_custom_command(
                OUTPUT ${outputs}
                DEPENDS ${PROTOS_TIMESTAMP} ${page} make_doc
                COMMAND ${MAKE_DOC} --to man -- ${page}
                WORKING_DIRECTORY ${MAN_DIR}
                )

            list(APPEND MAN_PAGES ${outputs})
        endif(lines)
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
    OUTPUT ${DUMMY_REFS}
    DEPENDS ${PAGES_TXT} make_dummy_refs
    COMMAND ${MAKE_DUMMY_REFS} ${PAGES_TXT} > ${DUMMY_REFS}
    )

add_custom_command(
    OUTPUT ${DUMMY_REFS_TIMESTAMP}
    DEPENDS ${DUMMY_REFS}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DUMMY_REFS} ${DUMMY_REFS_TIMESTAMP}
    )

add_custom_target(gen_dummy_refs DEPENDS ${DUMMY_REFS})

if(WANT_DOCS_INFO AND PANDOC_WITH_TEXINFO AND MAKEINFO)
    make_directory(${INFO_DIR})
    make_directory(${TEXI_DIR})

    add_custom_target(info ALL DEPENDS ${INFO_DIR}/refman.info)
    add_custom_command(
        OUTPUT ${INFO_DIR}/refman.info
        DEPENDS ${TEXI_DIR}/refman.texi
        COMMAND ${MAKEINFO}
                --paragraph-indent 0
                --no-split
                ${TEXI_DIR}/refman.texi
                -o ${INFO_DIR}/refman.info
        )
    add_custom_command(
        OUTPUT ${TEXI_DIR}/refman.texi
        DEPENDS ${PROTOS_TIMESTAMP} ${DUMMY_REFS_TIMESTAMP} ${PAGES_TXT}
                make_doc
        COMMAND ${MAKE_DOC}
                --to texinfo
                --standalone
                --
                ${DUMMY_REFS}
                ${PAGES_TXT}
                > ${TEXI_DIR}/refman.texi
        )
else()
    if(WANT_DOCS_INFO)
        message("Info documentation requires Pandoc 1.1+ and makeinfo")
    endif(WANT_DOCS_INFO)
endif(WANT_DOCS_INFO AND PANDOC_WITH_TEXINFO AND MAKEINFO)

#-----------------------------------------------------------------------------#
#
#   LaTeX (PDF)
#
#-----------------------------------------------------------------------------#

set(MAKE_PDF ${WANT_DOCS_PDF})

if(WANT_DOCS_PDF AND NOT PANDOC_FOR_LATEX)
    set(MAKE_PDF 0)
    message("PDF generation requires pandoc 1.5+")
endif()

if(WANT_DOCS_PDF AND NOT PDFLATEX_COMPILER)
    set(MAKE_PDF 0)
    message("PDF generation requires pdflatex")
endif()

if(MAKE_PDF)
    if(WANT_DOCS_PDF_PAPER)
        set(paperref 1)
    else()
        set(paperref)
    endif()

    make_directory(${LATEX_DIR})
    add_custom_target(latex ALL DEPENDS ${LATEX_DIR}/refman.tex)
    add_custom_command(
        OUTPUT ${LATEX_DIR}/refman.tex
        DEPENDS ${PROTOS_TIMESTAMP}
                ${DUMMY_REFS_TIMESTAMP}
                ${PAGES_TXT}
                ${SRC_REFMAN_DIR}/latex.template
                make_doc
        COMMAND ${MAKE_DOC}
                --to latex
                --template ${SRC_REFMAN_DIR}/latex.template
                -V paperref=${paperref}
                --standalone
                --toc
                --number-sections
                -- ${DUMMY_REFS} ${PAGES_TXT}
                > ${LATEX_DIR}/refman.tex
        )
    set(PDF_IMAGES)
    foreach(image ${IMAGES})
        add_custom_command(
            OUTPUT ${LATEX_DIR}/images/${image}.png
            DEPENDS ${SRC_REFMAN_DIR}/images/${image}.png
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${SRC_REFMAN_DIR}/images/${image}.png" "${LATEX_DIR}/images/${image}.png"
            )
        list(APPEND PDF_IMAGES ${LATEX_DIR}/images/${image}.png)
    endforeach(image)

    make_directory(${PDF_DIR})
    add_custom_target(pdf ALL DEPENDS ${PDF_DIR}/refman.pdf)
    add_custom_command(
        OUTPUT ${PDF_DIR}/refman.pdf
        DEPENDS ${LATEX_DIR}/refman.tex
        DEPENDS ${PDF_IMAGES}
        # Repeat three times to get cross references correct.
        COMMAND "${CMAKE_COMMAND}" -E chdir ${LATEX_DIR} ${PDFLATEX_COMPILER} -interaction batchmode -output-directory ${PDF_DIR} ${LATEX_DIR}/refman.tex
        COMMAND "${CMAKE_COMMAND}" -E chdir ${LATEX_DIR} ${PDFLATEX_COMPILER} -interaction batchmode -output-directory ${PDF_DIR} ${LATEX_DIR}/refman.tex
        COMMAND "${CMAKE_COMMAND}" -E chdir ${LATEX_DIR} ${PDFLATEX_COMPILER} -interaction batchmode -output-directory ${PDF_DIR} ${LATEX_DIR}/refman.tex
        )
endif(MAKE_PDF)

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
