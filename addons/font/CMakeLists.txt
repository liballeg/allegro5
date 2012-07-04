set(FONT_SOURCES font.c fontbmp.c stdfont.c text.c)

set(FONT_INCLUDE_FILES allegro5/allegro_font.h)

set_our_header_properties(${FONT_INCLUDE_FILES})

add_our_library(allegro_font
    "${FONT_SOURCES};${FONT_INCLUDE_FILES}"
    "-DALLEGRO_FONT_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set_our_framework_properties(allegro_font AllegroFont-${ALLEGRO_SOVERSION})

install_our_library(allegro_font)
install_our_headers(${FONT_INCLUDE_FILES})

set(FONT_LINK_WITH allegro_font PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
