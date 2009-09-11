set(COLOR_SOURCES color.c)

set(COLOR_INCLUDE_FILES
    allegro5/allegro_color.h
    )

add_our_library(allegro_color -${ALLEGRO_VERSION}
    "${COLOR_SOURCES}"
    "-DALLEGRO_COLOR_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set(COLOR_LINK_WITH allegro_color PARENT_SCOPE)

install(FILES ${COLOR_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
