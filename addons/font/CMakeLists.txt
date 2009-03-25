set(FONT_SOURCES font.c fontbmp.c text.c)

set(FONT_INCLUDE_FILES allegro5/a5_font.h)

include_directories(../iio)

add_our_library(a5_font -${ALLEGRO_VERSION}
    "${FONT_SOURCES}"
    "-DALLEGRO_FONT_SRC"
    "${IIO_LINK_WITH}"
    )

set(FONT_LINK_WITH a5_font PARENT_SCOPE)

install(FILES ${FONT_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
