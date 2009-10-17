#-----------------------------------------------------------------------------#
#
# logg
#

find_package(Vorbis)
if(NOT VORBIS_FOUND)
    return()
endif()

include_directories(SYSTEM ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})

set(LOGG_SOURCES
    logg.c
    )

set(LOGG_HEADERS
    logg.h
    )

add_our_library(logg STATIC ${LOGG_SOURCES})
target_link_libraries(logg alleg ${VORBIS_LIBRARIES})

install(FILES ${LOGG_HEADERS}
        DESTINATION include
        )

#-----------------------------------------------------------------------------#
#
# Examples
#

if(WANT_EXAMPLES)
    add_executable(play_ogg play_ogg.c)
    target_link_libraries(play_ogg logg)

    add_executable(stream_ogg stream_ogg.c)
    target_link_libraries(stream_ogg logg)
endif(WANT_EXAMPLES)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
