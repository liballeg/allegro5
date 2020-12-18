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

set(DOC_GIT_REF "" CACHE STRING "Git ref to use for source links in the documentation. If empty, will query git for this.")

set(REAL_DOC_GIT_REF "")
if(DOC_GIT_REF)
    set(REAL_DOC_GIT_REF ${DOC_GIT_REF})
else()
    find_package(Git)
    if(GIT_FOUND)
        message(STATUS "Using git to determine git ref for documentation.")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_RESULT
            OUTPUT_VARIABLE GIT_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(GIT_RESULT EQUAL 0)
            set(REAL_DOC_GIT_REF ${GIT_OUTPUT})
        endif()
    endif()
endif()

if(NOT REAL_DOC_GIT_REF)
    set(REAL_DOC_GIT_REF master)
endif()

message(STATUS "Using ${REAL_DOC_GIT_REF} as the git ref for source links in the documentation.")

#-----------------------------------------------------------------------------#

# Pandoc 1.10 changed handling of internal cross-references in Texinfo writer,
# and LaTeX writer thereabouts.
if(PANDOC)
    message(STATUS "Checking Pandoc version")
    execute_process(
        COMMAND ${PANDOC} --version
        OUTPUT_VARIABLE PANDOC_VERSION_TEXT
        ERROR_VARIABLE PANDOC_VERSION_TEXT
        )
    if(PANDOC_VERSION_TEXT MATCHES "pandoc(.exe)? (1[.]1[0-9]|1[.][2-9][0-9]|[2-9])")
        # message(STATUS "Pandoc version is compatible")
    else()
        message(STATUS "Pandoc version is incompatible")
        set(PANDOC 0)
    endif()
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
        COMMAND ${PANDOC} ${abs_source_native} --from markdown --toc --standalone ${ARGN}
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
    pandoc(src/changes-5.0.txt html/changes-5.0.html -c pandoc.css)
    pandoc(src/changes-5.1.txt html/changes-5.1.html -c pandoc.css)
    pandoc(src/changes-5.2.txt html/changes-5.2.html -c pandoc.css)

    pandoc(src/changes-5.0.txt texi/changes-5.0.texi)
    pandoc(src/changes-5.1.txt texi/changes-5.1.texi)
    pandoc(src/changes-5.2.txt texi/changes-5.2.texi)
    if(MAKEINFO)
        texi2text(texi/changes-5.0.texi txt/changes-5.0.txt)
        texi2text(texi/changes-5.1.texi txt/changes-5.1.txt)
        texi2text(texi/changes-5.2.texi txt/changes-5.2.txt)
    endif(MAKEINFO)
endif(PANDOC)

add_custom_target(docs
    ALL
    DEPENDS ${all_docs}
    )

#-----------------------------------------------------------------------------#

make_directory(${CMAKE_CURRENT_BINARY_DIR}/html/refman)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/txt)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/texi)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/latex)

# Stick the ALLEGRO_VERSION into a file included by the latex template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/refman/allegro_version.tex.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/latex/allegro_version.tex
    @ONLY
    )

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
