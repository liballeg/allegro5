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

add_our_library(loadpng STATIC ${LOADPNG_SOURCES})
target_link_libraries(loadpng alleg ${PNG_LIBRARIES})

install(FILES ${LOADPNG_HEADERS}
        DESTINATION include
        )

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
