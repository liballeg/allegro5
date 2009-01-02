set(COLOR_SOURCES color.c)

set(COLOR_INCLUDE_FILES
    allegro5/a5_color.h
    )

add_our_library(a5_color -${ALLEGRO_VERSION}
    "${COLOR_SOURCES}"
    "-DA5_COLOR_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set(COLOR_LINK_WITH a5_color PARENT_SCOPE)

install(FILES ${COLOR_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
