set(FONT_SOURCES font.c fontbmp.c stdfont.c text.c bmfont.c xml.c)

set(FONT_INCLUDE_FILES allegro5/allegro_font.h)

set_our_header_properties(${FONT_INCLUDE_FILES})

add_our_addon_library(allegro_font
    AllegroFont-${A5O_SOVERSION}
    "${FONT_SOURCES};${FONT_INCLUDE_FILES}"
    "-DA5O_FONT_SRC"
    "${A5O_LINK_WITH}"
    )

install_our_headers(${FONT_INCLUDE_FILES})

add_addon(font)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
