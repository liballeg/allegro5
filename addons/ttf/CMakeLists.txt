set(TTF_SOURCES ttf.c)

set(TTF_INCLUDE_FILES allegro5/allegro_ttf.h)

configure_file(
    allegro5/internal/aintern_ttf_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_ttf_cfg.h
    )

include_directories(../font)
include_directories(${FREETYPE_INCLUDE_DIRS})

add_our_library(allegro_ttf -${ALLEGRO_VERSION}
    "${TTF_SOURCES}"
    "-DA5_TTF_SRC"
    "${FONT_LINK_WITH};${FREETYPE_LIBRARIES}"
    )

set(TTF_LINK_WITH allegro_ttf PARENT_SCOPE)

install(FILES ${TTF_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
