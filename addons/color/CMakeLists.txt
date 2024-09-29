set(COLOR_SOURCES color.c)

set(COLOR_INCLUDE_FILES
    allegro5/allegro_color.h
    )

set_our_header_properties(${COLOR_INCLUDE_FILES})

add_our_addon_library(allegro_color
    AllegroColor-${A5O_SOVERSION}
    "${COLOR_SOURCES};${COLOR_INCLUDE_FILES}"
    "-DA5O_COLOR_SRC"
    "${A5O_LINK_WITH}"
    )

install_our_headers(${COLOR_INCLUDE_FILES})

add_addon(color)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
