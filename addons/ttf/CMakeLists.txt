include_directories(../font)
include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})

set(TTF_SOURCES ttf.c)

set(TTF_INCLUDE_FILES allegro5/allegro_ttf.h)

set_our_header_properties(${TTF_INCLUDE_FILES})

configure_file(
    allegro5/internal/aintern_ttf_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_ttf_cfg.h
    )

set(TTF_LIBRARIES ${FREETYPE_LIBRARIES})
set(TTF_INCLUDE_DIRECTORIES ${FREETYPE_INCLUDE_DIRS})

# FREETYPE_LIBRARIES should include zlib automatically if statically linking,
# but it doesn't.
find_package(ZLIB)
if(ZLIB_FOUND AND NOT IPHONE)
    list(APPEND TTF_LIBRARIES ${ZLIB_LIBRARIES})
endif()

add_our_addon_library(allegro_ttf
    AllegroTTF-${ALLEGRO_SOVERSION}
    "${TTF_SOURCES};${TTF_INCLUDE_FILES}"
    "-DALLEGRO_TTF_SRC"
    "${FONT_LINK_WITH};${TTF_LIBRARIES}"
    )

install_our_headers(${TTF_INCLUDE_FILES})

add_addon(ttf)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
