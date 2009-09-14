set(FONT_SOURCES font.c fontbmp.c text.c)

set(FONT_INCLUDE_FILES allegro5/allegro_font.h)

include_directories(../image)
include_directories(../primitives)

add_our_library(allegro_font -${ALLEGRO_VERSION}
    "${FONT_SOURCES}"
    "-DALLEGRO_FONT_SRC"
    "${IMAGE_LINK_WITH};${PRIMITIVES_LINK_WITH}"
    )

set(FONT_LINK_WITH allegro_font PARENT_SCOPE)

install(FILES ${FONT_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
