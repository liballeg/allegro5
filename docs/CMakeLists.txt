find_program(MAKEINFO NAMES makeinfo)
mark_as_advanced(MAKEINFO)

find_program(PANDOC NAMES pandoc)
mark_as_advanced(PANDOC)

# sh is only used for a developer-only target.
find_program(SH NAMES ash dash sh)
mark_as_advanced(SH)

find_package(LATEX)

find_program(CTAGS NAMES ctags)
mark_as_advanced(CTAGS)

#-----------------------------------------------------------------------------#

# Pandoc < 1.0 is usable but doesn't have Texinfo support.
if(PANDOC AND NOT DEFINED PANDOC_WITH_TEXINFO)
    message(STATUS "Checking Pandoc version")
    execute_process(
        COMMAND ${PANDOC} --version
        OUTPUT_VARIABLE PANDOC_VERSION_TEXT
        ERROR_VARIABLE PANDOC_VERSION_TEXT
        )
    if(PANDOC_VERSION_TEXT MATCHES "pandoc [1-9]")
        set(PANDOC_WITH_TEXINFO 1 CACHE BOOL "Pandoc supports Texinfo")
    else()
        set(PANDOC_WITH_TEXINFO 0 CACHE BOOL "Pandoc supports Texinfo")
    endif()
    mark_as_advanced(PANDOC_WITH_TEXINFO)
endif()

# Pandoc 1.2 and 1.2.1 accidentally changed the way it generates HTML
# identifiers.
if(PANDOC AND NOT DEFINED PANDOC_STRIP_UNDERSCORES)
    if(NOT PANDOC_VERSION_TEXT)
        execute_process(
            COMMAND ${PANDOC} --version
            OUTPUT_VARIABLE PANDOC_VERSION_TEXT
            ERROR_VARIABLE PANDOC_VERSION_TEXT
            )
    endif()
    if(PANDOC_VERSION_TEXT MATCHES "pandoc 1[.]2(\n|[.]1)")
        set(PANDOC_STRIP_UNDERSCORES 1 CACHE BOOL
            "Pandoc strips underscores for HTML identifiers")
    else()
        set(PANDOC_STRIP_UNDERSCORES 0 CACHE BOOL
            "Pandoc strips underscores for HTML identifiers")
    endif()
    mark_as_advanced(PANDOC_STRIP_UNDERSCORES)
endif()

# Pandoc 1.5 or later is required for LaTeX as it will begin
# sectioning with \chapter.
if(PANDOC AND NOT DEFINED PANDOC_FOR_LATEX)
    if(NOT PANDOC_VERSION_TEXT)
        execute_process(
            COMMAND ${PANDOC} --version
            OUTPUT_VARIABLE PANDOC_VERSION_TEXT
            ERROR_VARIABLE PANDOC_VERSION_TEXT
            )
    endif()
    if(PANDOC_VERSION_TEXT MATCHES "pandoc (0|1[.][01234])")
        set(PANDOC_FOR_LATEX 0 CACHE BOOL
            "Pandoc recent enough for LaTeX output")
    else()
        set(PANDOC_FOR_LATEX 1 CACHE BOOL
            "Pandoc recent enough for LaTeX output")
    endif()
    mark_as_advanced(PANDOC_FOR_LATEX)
endif()

#-----------------------------------------------------------------------------#

set(all_docs)

macro(add_info n)
    if(MAKEINFO)
        makedoc(src/${n}._tx -texi texi/${n}.texi)

        set(abs_info ${CMAKE_CURRENT_BINARY_DIR}/info/${n}.info)
        set(abs_texi ${CMAKE_CURRENT_BINARY_DIR}/texi/${n}.texi)

        list(APPEND all_docs ${abs_info})
        add_custom_command(
            OUTPUT  ${abs_info}
            DEPENDS ${abs_texi}
            COMMAND ${MAKEINFO} --no-split -o ${abs_info} ${abs_texi}
            )
    endif(MAKEINFO)
endmacro(add_info)

#-----------------------------------------------------------------------------#

function(pandoc source output) # extraargs...
    set(abs_source ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    set(abs_output ${CMAKE_CURRENT_BINARY_DIR}/${output})

    # Use native Windows syntax to avoid "c:/foo.txt" being treated as a
    # remote URI by Pandoc 1.5 and 1.6.
    file(TO_NATIVE_PATH ${abs_source} abs_source_native)

    list(APPEND all_docs ${abs_output})
    set(all_docs ${all_docs} PARENT_SCOPE)

    add_custom_command(
        OUTPUT  ${abs_output}
        DEPENDS ${abs_source}
        COMMAND ${PANDOC} ${abs_source_native} --toc --standalone ${ARGN}
                -o ${abs_output}
        )
endfunction(pandoc source output)

function(texi2text source output)
    # The source file is a generated Texinfo file.
    set(abs_source ${CMAKE_CURRENT_BINARY_DIR}/${source})
    set(abs_output ${CMAKE_CURRENT_BINARY_DIR}/${output})

    list(APPEND all_docs ${abs_output})
    set(all_docs ${all_docs} PARENT_SCOPE)

    # Writing to stdout suppresses the table of contents.
    # To get the table of contents, use `makeinfo -o ${output}`.
    add_custom_command(
        OUTPUT  ${abs_output}
        DEPENDS ${abs_source}
        COMMAND ${MAKEINFO}
                --plaintext
                --paragraph-indent 0
                --no-number-sections
                ${abs_source} > ${abs_output}
        )
endfunction(texi2text)

if(PANDOC)
    pandoc(
        src/changes-5.0.txt
        html/changes-5.0.html
        -c pandoc.css
        )
endif(PANDOC)

if(PANDOC_WITH_TEXINFO)
    pandoc(
        src/changes-5.0.txt
        texi/changes-5.0.texi
        )
    if(MAKEINFO)
        texi2text(
            texi/changes-5.0.texi
            txt/changes-5.0.txt
            )
    endif(MAKEINFO)
endif(PANDOC_WITH_TEXINFO)

add_custom_target(docs
    ALL
    DEPENDS ${all_docs}
    )

#-----------------------------------------------------------------------------#

make_directory(${CMAKE_CURRENT_BINARY_DIR}/html/refman)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/txt)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/texi)

# Copy CSS files.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pandoc.css
    ${CMAKE_CURRENT_BINARY_DIR}/html/pandoc.css
    COPYONLY
    )
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pandoc.css
    ${CMAKE_CURRENT_BINARY_DIR}/html/refman/pandoc.css
    COPYONLY
    )

if(PANDOC AND NOT CMAKE_CROSSCOMPILING)
    include(Refman.cmake)
endif()

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
