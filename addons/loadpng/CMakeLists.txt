#-----------------------------------------------------------------------------#
#
# loadpng
#

find_package(PNG)
if(NOT PNG_FOUND)
    return()
endif()

include_directories(SYSTEM ${PNG_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(LOADPNG_SOURCES
    loadpng.c
    savepng.c
    regpng.c
    )

set(LOADPNG_HEADERS
    loadpng.h
    )

set_source_files_properties(${LOADPNG_HEADERS}
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Headers
    )

add_our_library(loadpng ${ADDON_LINKAGE}
    ${LOADPNG_SOURCES}
    ${LOADPNG_HEADERS}
    )

set_target_properties(loadpng
    PROPERTIES
    LINK_FLAGS "${ALLEGRO_LINK_FLAGS}"
    VERSION ${ALLEGRO_VERSION}
    SOVERSION ${ALLEGRO_SOVERSION}
    )

set_our_framework_properties(loadpng loadpng)

if(PSP)
    target_link_libraries(loadpng ${PNG_LIBRARIES} allegro)
endif(PSP)

target_link_libraries(loadpng allegro ${PNG_LIBRARIES})

sanitize_cmake_link_flags(static_link_with ${PNG_LIBRARIES})
set_target_properties(loadpng
    PROPERTIES
    static_link_with "${static_link_with}"
    )

install_our_library(loadpng)
if(NOT WANT_FRAMEWORKS)
    install_our_headers(${LOADPNG_HEADERS})
endif(NOT WANT_FRAMEWORKS)

#-----------------------------------------------------------------------------#
#
# Examples
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples)

function(add_our_example nm)
    add_executable(${nm}_png WIN32 examples/${nm}.c)
    set_target_properties(${nm}_png
        PROPERTIES
        OUTPUT_NAME examples/${nm}
        )
    target_link_libraries(${nm}_png loadpng)
    if(PSP)
        add_psp_executable(${nm}_png)
    endif(PSP)
endfunction()

if(WANT_EXAMPLES)
    add_our_example(example)
    add_our_example(exalpha)
    add_our_example(exdata)
    add_our_example(browse)

    copy_files(copy_loadpng_example_data
        examples/alpha.png
        examples/exdata.dat
        )
endif(WANT_EXAMPLES)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
