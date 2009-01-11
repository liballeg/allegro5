find_program(MAKEINFO NAMES makeinfo)
mark_as_advanced(MAKEINFO)

find_program(PANDOC NAMES pandoc)
mark_as_advanced(PANDOC)

# Our scripts run faster under ash than bash, but ash is probably not
# the default shell.
find_program(SH NAMES ash dash sh)
find_program(AWK NAMES awk)
find_program(GREP NAMES grep)
find_program(SED NAMES sed)
find_program(CUT NAMES cut)
mark_as_advanced(SH AWK GREP SED CUT)

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

    list(APPEND all_docs ${abs_output})
    set(all_docs ${all_docs} PARENT_SCOPE)

    add_custom_command(
        OUTPUT  ${abs_output}
        DEPENDS ${abs_source}
        COMMAND ${PANDOC} ${abs_source} --toc --standalone ${ARGN}
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
        src/changes-4.9.txt
        html/changes-4.9.html
        -c pandoc.css
        )
    pandoc(
        src/changes-4.9.txt
        texi/changes-4.9.texi
        )
    if(MAKEINFO)
        texi2text(
            texi/changes-4.9.texi
            txt/changes-4.9.txt
            )
    endif(MAKEINFO)
    pandoc(
        src/refman/threads.txt
        html/refman/threads.html
        -c pandoc.css
        )
endif(PANDOC)

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

#-----------------------------------------------------------------------------#

# The reference manual uses a documentation system which currently
# requires a small POSIXy environment.

if(PANDOC AND SH AND AWK AND GREP AND SED AND CUT)
    add_subdirectory(src/refman)
endif()

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
