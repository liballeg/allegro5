#-----------------------------------------------------------------------------#
#
# Documentation
#

option(WANT_DOCS "Generate documentation" on)
if(NOT WANT_DOCS OR CMAKE_CROSSCOMPILING)
    return()
endif()

set(DOCDIR "doc" CACHE STRING "Install docs into $DOCDIR/allegro-$VERSION")
set(ACTUAL_DOC_DIR "${DOCDIR}/allegro-${ALLEGRO_VERSION}")

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

file(MAKE_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/build
    ${CMAKE_CURRENT_BINARY_DIR}/txt
    ${CMAKE_CURRENT_BINARY_DIR}/html
    ${CMAKE_CURRENT_BINARY_DIR}/rtf
    ${CMAKE_CURRENT_BINARY_DIR}/scite
    ${CMAKE_CURRENT_BINARY_DIR}/texi
    ${CMAKE_CURRENT_BINARY_DIR}/info
    ${CMAKE_CURRENT_BINARY_DIR}/man
    )

set(DOC_SRCS
    src/abi._tx
    src/addons._tx
    src/ahack._tx
    src/allegro._tx
    src/api._tx
    src/changes._tx
    src/const._tx
    src/dat2c._tx
    src/dat2s._tx
    src/datafile._tx
    src/dat._tx
    src/faq._tx
    src/grabber._tx
    src/help._tx
    src/index._tx
    src/license._tx
    src/makedoc._tx
    src/mistakes._tx
    src/packfile._tx
    src/readme._tx
    src/thanks._tx

    src/build/cmake._tx
    src/build/linux._tx
    src/build/macosx._tx
    src/build/mingw32._tx
    src/build/msvc._tx
    src/build/unix._tx
    )

set(all_docs)
foreach(page ${DOC_SRCS})
    string(REGEX MATCH "[^/]*._tx" basename ${page})
    string(REPLACE "._tx" "" basename ${basename})

    set(page ${CMAKE_CURRENT_SOURCE_DIR}/${page})
    if(${page} MATCHES "/build/")
        set(txt_out ${CMAKE_CURRENT_BINARY_DIR}/build/${basename}.txt)
        set(html_out ${CMAKE_CURRENT_BINARY_DIR}/build/${basename}.html)
    else()
        set(txt_out ${CMAKE_CURRENT_BINARY_DIR}/txt/${basename}.txt)
        set(html_out ${CMAKE_CURRENT_BINARY_DIR}/html/${basename}.html)
    endif()

    add_custom_command(
        OUTPUT ${txt_out}
        DEPENDS makedoc ${page}
        COMMAND makedoc -ascii ${txt_out} ${page}
        )

    add_custom_command(
        OUTPUT ${html_out}
        DEPENDS makedoc ${page}
        COMMAND makedoc -html ${html_out} ${page}
        )

    list(APPEND all_docs ${txt_out} ${html_out})
endforeach()

macro(ascii_file out src)
    # You'll have to ask Shawn why it was like this...
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${out}
        DEPENDS makedoc ${CMAKE_CURRENT_SOURCE_DIR}/${src}
        COMMAND makedoc ${ARGN}
                -ascii ${CMAKE_CURRENT_BINARY_DIR}/${out}
                ${CMAKE_CURRENT_SOURCE_DIR}/${src}
        )
    list(APPEND all_docs ${CMAKE_CURRENT_BINARY_DIR}/${out})
endmacro()

# Files in the root directory.
ascii_file(AUTHORS src/thanks._tx -part)
ascii_file(THANKS src/thanks._tx -part)
ascii_file(CHANGES src/changes._tx)
ascii_file(readme.txt src/readme._tx)

if(UNIX)
    set(TEXT_FILES_TO_INSTALL
        ${CMAKE_CURRENT_BINARY_DIR}/AUTHORS
        ${CMAKE_CURRENT_BINARY_DIR}/THANKS
        ${CMAKE_CURRENT_BINARY_DIR}/CHANGES
        ${CMAKE_CURRENT_BINARY_DIR}/readme.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/abi.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/addons.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/ahack.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/allegro.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/api.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/const.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/dat.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/dat2c.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/dat2s.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/datafile.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/faq.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/grabber.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/help.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/license.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/makedoc.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/mistakes.txt
        ${CMAKE_CURRENT_BINARY_DIR}/txt/packfile.txt
        )
    install(FILES ${TEXT_FILES_TO_INSTALL}
            DESTINATION ${ACTUAL_DOC_DIR}
            )

    file(GLOB HTML_DOCS
            ${CMAKE_CURRENT_BINARY_DIR}/html/*.html
            ${CMAKE_CURRENT_BINARY_DIR}/html/*.css
            )
    install(FILES ${HTML_DOCS}
            DESTINATION ${ACTUAL_DOC_DIR}/html
            )
endif(UNIX)

#-----------------------------------------------------------------------------#
#
# Man pages
#

option(WANT_DOCS_MAN "Generate man pages" on)
set(MANDIR "man" CACHE STRING "Install man pages into this directory")

if(WANT_DOCS_MAN)
    set(man_out ${CMAKE_CURRENT_BINARY_DIR}/man/allegro_init.3)
    set(man_src ${CMAKE_CURRENT_SOURCE_DIR}/src/allegro._tx)

    add_custom_command(
        OUTPUT ${man_out}
        DEPENDS makedoc ${man_src}
        COMMAND makedoc -man ${man_out} ${man_src}
        )
    list(APPEND all_docs ${man_out})

    # I'm assuming only Unix users want man pages installed, but djgpp supports
    # man pages to so this might need to change.
    if(UNIX)
        file(GLOB MAN_PAGES ${CMAKE_CURRENT_BINARY_DIR}/man/*.3)
        install(FILES ${MAN_PAGES}
                DESTINATION ${MANDIR}/man3
                )
    endif(UNIX)
endif(WANT_DOCS_MAN)

#-----------------------------------------------------------------------------#
#
# Info
#

option(WANT_DOCS_INFO "Generate Info document" on)
find_program(MAKEINFO NAMES makeinfo)
mark_as_advanced(MAKEINFO)
set(INFODIR "info" CACHE STRING "Install Info docs into this directory")

if(WANT_DOCS_INFO AND MAKEINFO)
    set(texi_out ${CMAKE_CURRENT_BINARY_DIR}/texi/allegro.texi)
    set(info_out ${CMAKE_CURRENT_BINARY_DIR}/info/allegro.info)
    set(info_src ${CMAKE_CURRENT_SOURCE_DIR}/src/allegro._tx)

    add_custom_command(
        OUTPUT ${texi_out}
        DEPENDS makedoc ${info_src}
        COMMAND makedoc -texi ${texi_out} ${info_src}
        )
    add_custom_command(
        OUTPUT ${info_out}
        DEPENDS ${texi_out}
        COMMAND ${MAKEINFO} --no-split -o ${info_out} ${texi_out}
        )
    list(APPEND all_docs ${info_out})

    if(UNIX)
        install(FILES ${info_out}
                DESTINATION ${INFODIR}
                )
    endif(UNIX)
endif(WANT_DOCS_INFO AND MAKEINFO)

#-----------------------------------------------------------------------------#
#
# Scite
#

set(scite_out ${CMAKE_CURRENT_BINARY_DIR}/scite/allegro.api)
set(scite_src ${CMAKE_CURRENT_SOURCE_DIR}/src/allegro._tx)

add_custom_command(
    OUTPUT ${scite_out}
    DEPENDS makedoc ${page}
    COMMAND makedoc -scite ${scite_out} ${scite_src}
    )
list(APPEND all_docs ${scite_out})

#-----------------------------------------------------------------------------#

add_custom_target(docs ALL DEPENDS ${all_docs})

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
