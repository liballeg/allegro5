include_directories(../font)
include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})

set(TTF_SOURCES ttf.c)

set(TTF_INCLUDE_FILES allegro5/allegro_ttf.h)

set_our_header_properties(${TTF_INCLUDE_FILES})

configure_file(
    allegro5/internal/aintern_ttf_cfg.h.cmake
    ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_ttf_cfg.h
    )

add_our_addon_library(allegro_ttf
    AllegroTTF-${A5O_SOVERSION}
    "${TTF_SOURCES};${TTF_INCLUDE_FILES}"
    "-DA5O_TTF_SRC"
    "${FONT_LINK_WITH};${TTF_LIBRARIES}"
    )

install_our_headers(${TTF_INCLUDE_FILES})

add_addon(ttf)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
