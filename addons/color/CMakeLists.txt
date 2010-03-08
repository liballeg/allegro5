set(COLOR_SOURCES color.c)

set(COLOR_INCLUDE_FILES
    allegro5/allegro_color.h
    )

set_our_header_properties(${COLOR_INCLUDE_FILES})

add_our_library(allegro_color
    "${COLOR_SOURCES};${COLOR_INCLUDE_FILES}"
    "-DALLEGRO_COLOR_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set_our_framework_properties(allegro_color AllegroColor-${ALLEGRO_SOVERSION})

install_our_library(allegro_color)
install_our_headers(${COLOR_INCLUDE_FILES})

set(COLOR_LINK_WITH allegro_color PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
