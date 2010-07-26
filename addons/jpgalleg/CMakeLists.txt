#-----------------------------------------------------------------------------#
#
# JPGalleg
#

# JPGalleg contains some optional assembly code for x86.
# In the end I decided not to enable it as we can't be sure we can maintain
# it in the future, so this file never contains anything useful.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mmx.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/mmx.h
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

set(JPGALLEG_SOURCES
    src/decode.c
    src/encode.c
    src/error.c
    src/init.c
    src/io.c
    src/jpgalleg.c
    )

set(JPGALLEG_HEADERS
    include/jpgalleg.h
    )

set_source_files_properties(${JPGALLEG_HEADERS}
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Headers
    )

add_our_library(jpgalleg ${ADDON_LINKAGE}
    ${JPGALLEG_SOURCES}
    ${JPGALLEG_HEADERS}
    )

set_target_properties(jpgalleg
    PROPERTIES
    LINK_FLAGS "${ALLEGRO_LINK_FLAGS}"
    VERSION ${ALLEGRO_VERSION}
    SOVERSION ${ALLEGRO_SOVERSION}
    )

set_our_framework_properties(jpgalleg jpgalleg)

target_link_libraries(jpgalleg allegro)

set_target_properties(jpgalleg
    PROPERTIES
    static_link_with ""
    )

install_our_library(jpgalleg)
if(NOT WANT_FRAMEWORKS)
    install_our_headers(${JPGALLEG_HEADERS})
endif(NOT WANT_FRAMEWORKS)

set(SUPPORT_JPGALLEG 1 PARENT_SCOPE)

#-----------------------------------------------------------------------------#
#
# Examples
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples)

function(add_our_example nm)
    add_executable(${nm}_jpg WIN32 examples/${nm}.c)
    set_target_properties(${nm}_jpg
        PROPERTIES
        OUTPUT_NAME examples/${nm}
        )
    target_link_libraries(${nm}_jpg jpgalleg)
    if(PSP)
        add_psp_executable(${nm}_jpg)
    endif(PSP)
endfunction()

if(WANT_EXAMPLES)
    add_our_example(ex1)
    add_our_example(ex2)
    add_our_example(ex3)
    add_our_example(ex4)
    add_our_example(ex5)

    copy_files(copy_jpgalleg_example_data
        examples/cat.tga
        examples/datafile.dat
        examples/jpgalleg.jpg
        )
endif(WANT_EXAMPLES)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
