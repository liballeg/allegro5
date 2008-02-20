find_program(MAKEINFO NAMES makeinfo)
mark_as_advanced(MAKEINFO)

#-----------------------------------------------------------------------------#

set(all_docs)

add_executable(makedoc
    src/makedoc/makedoc.c
    src/makedoc/makehtml.c
    src/makedoc/makemisc.c
    src/makedoc/maketxt.c
    src/makedoc/makertf.c
    src/makedoc/maketexi.c
    src/makedoc/makeman.c
    src/makedoc/makechm.c
    src/makedoc/makedevh.c
    src/makedoc/makesci.c
    )
get_target_property(MAKEDOC_EXE makedoc LOCATION)

macro(makedoc source mode output)
    set(abs_source ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    set(abs_output ${CMAKE_CURRENT_BINARY_DIR}/${output})

    list(APPEND all_docs ${abs_output})
    add_custom_command(
        OUTPUT  ${abs_output}
        COMMAND ${MAKEDOC_EXE} ${mode} ${abs_output} ${abs_source}
        DEPENDS makedoc ${abs_source}
        )
endmacro(makedoc source mode output)

macro(add_document n)
    makedoc(src/${n}._tx -ascii txt/${n}.txt)
    makedoc(src/${n}._tx -html html/${n}.html)
    makedoc(src/${n}._tx -rtf rtf/${n}.rtf)
endmacro(add_document)

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

macro(add_man_pages n one)
    makedoc(src/${n}._tx -man man/${one}.3)
endmacro(add_man_pages n)

macro(add_build_document n)
    makedoc(src/build/${n}._tx -ascii build/${n}.txt)
    makedoc(src/build/${n}._tx -html html/build/${n}.html)
endmacro(add_build_document)

add_document(abi)
add_document(ahack)
add_document(allegro)
add_document(api)
add_document(changes)
add_document(const)
add_document(dat)
add_document(dat2c)
add_document(dat2s)
add_document(datafile)
add_document(faq)
add_document(grabber)
add_document(help)
add_document(license)
add_document(makedoc)
add_document(mistakes)
add_document(packfile)
add_document(readme)
add_document(thanks)

add_info(allegro)

add_man_pages(allegro allegro_init)

add_build_document(bcc32)
add_build_document(beos)
add_build_document(darwin)
add_build_document(djgpp)
add_build_document(linux)
add_build_document(macosx)
add_build_document(mingw32)
add_build_document(msvc)
add_build_document(qnx)
add_build_document(unix)
add_build_document(watcom)

add_custom_target(docs
    ALL
    DEPENDS ${all_docs}
    )

#-----------------------------------------------------------------------------#

    # Recreate directory structure for out-of-source builds.
    #
make_directory(${CMAKE_CURRENT_BINARY_DIR}/build)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/html/build)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/info)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/man)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/rtf)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/texi)
make_directory(${CMAKE_CURRENT_BINARY_DIR}/txt)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
