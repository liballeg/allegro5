set(COLOR_SOURCES color.c)

set(COLOR_INCLUDE_FILES
    allegro5/allegro_color.h
    )

set_our_header_properties(${COLOR_INCLUDE_FILES})

add_our_addon_library(allegro_color
    AllegroColor-${ALLEGRO_SOVERSION}
    "${COLOR_SOURCES};${COLOR_INCLUDE_FILES}"
    "-DALLEGRO_COLOR_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

install_our_headers(${COLOR_INCLUDE_FILES})

add_addon(color)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
