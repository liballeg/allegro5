find_program(MAKEINFO NAMES makeinfo)
mark_as_advanced(MAKEINFO)

find_program(PANDOC NAMES pandoc)
mark_as_advanced(PANDOC)

find_program(LINKS NAMES links elinks)
mark_as_advanced(LINKS)

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
        COMMAND ${PANDOC} ${abs_source} --toc --standalone ${ARGN}
                -o ${abs_output}
        DEPENDS ${abs_source}
        )
endfunction(pandoc source output)

# We can actually produce nicer plain text output by generating Texinfo
# then calling makeinfo --plaintext.  But the current release (pandoc-0.46)
# doesn't yet include a Texinfo backend so for now we use links -dump.
function(links source output)
    # The source file is a generated HTML file.
    set(abs_source ${CMAKE_CURRENT_BINARY_DIR}/${source})
    set(abs_output ${CMAKE_CURRENT_BINARY_DIR}/${output})

    list(APPEND all_docs ${abs_output})
    set(all_docs ${all_docs} PARENT_SCOPE)

    add_custom_command(
        OUTPUT  ${abs_output}
        COMMAND ${LINKS} -dump ${abs_source} > ${abs_output}
        DEPENDS ${abs_source}
        )
endfunction(links source output)

if(PANDOC)
    pandoc(
        src/changes-4.9.txt
        html/changes-4.9.html
        -c pandoc.css
    )
    if(LINKS)
        links(
            html/changes-4.9.html
            txt/changes-4.9.txt
        )
    endif(LINKS)

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
# vi: set ts=8 sts=4 sw=4 et:
