include_directories(../font)
include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})

set(TTF_SOURCES ttf.c)

set(TTF_INCLUDE_FILES allegro5/allegro_ttf.h)

set_our_header_properties(${TTF_INCLUDE_FILES})

configure_file(
    allegro5/internal/aintern_ttf_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_ttf_cfg.h
    )

# FREETYPE_LIBRARIES should include zlib automatically if statically linking,
# but it doesn't.
find_package(ZLIB)
if(ZLIB_FOUND AND NOT IPHONE)
    list(APPEND FREETYPE_LIBRARIES ${ZLIB_LIBRARIES})
endif()

add_our_library(allegro_ttf
    "${TTF_SOURCES};${TTF_INCLUDE_FILES}"
    "-DALLEGRO_TTF_SRC"
    "${FONT_LINK_WITH};${FREETYPE_LIBRARIES}"
    )

set_our_framework_properties(allegro_ttf AllegroTTF-${ALLEGRO_SOVERSION})

install_our_library(allegro_ttf)
install_our_headers(${TTF_INCLUDE_FILES})

set(TTF_LINK_WITH allegro_ttf PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
