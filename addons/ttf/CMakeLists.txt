include_directories(../font)
include_directories(SYSTEM ${FREETYPE_INCLUDE_DIRS})

set(TTF_SOURCES ttf.c)

set(TTF_INCLUDE_FILES allegro5/allegro_ttf.h)

set_our_header_properties(${TTF_INCLUDE_FILES})

configure_file(
    allegro5/internal/aintern_ttf_cfg.h.cmake
    ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_ttf_cfg.h
    )

set(TTF_LIBRARIES ${FREETYPE_LIBRARIES})
set(TTF_INCLUDE_DIRECTORIES ${FREETYPE_INCLUDE_DIRS})

set(CMAKE_REQUIRED_INCLUDES ${FREETYPE_INCLUDE_DIRS})

check_c_source_compiles("
    #include <ft2build.h>
    #include FT_FREETYPE_H
    #ifndef FT_CONFIG_OPTION_SYSTEM_ZLIB
    #error
    #endif
    int main(void) { return 0; }"
    FREETYPE_NEED_ZLIB)

if(FREETYPE_NEED_ZLIB)
    find_package(ZLIB)
    if(ZLIB_FOUND)
        list(APPEND TTF_LIBRARIES ${ZLIB_LIBRARIES})
    else()
        message(WARNING "FreeType was compiled with Zlib support, "
                        "but no Zlib library found.")
    endif()
endif()

check_c_source_compiles("
    #include <ft2build.h>
    #include FT_FREETYPE_H
    #ifndef FT_CONFIG_OPTION_USE_BZIP2
    #error
    #endif
    int main(void) { return 0; }"
    FREETYPE_NEED_BZip2)

if(FREETYPE_NEED_BZip2)
    find_package(BZip2)
    if(BZIP2_FOUND)
        list(APPEND TTF_LIBRARIES ${BZIP2_LIBRARIES})
    else()
        message(WARNING "FreeType was compiled with BZip2 support, "
                        "but no BZip2 library found.")
    endif()
endif()

check_c_source_compiles("
    #include <ft2build.h>
    #include FT_FREETYPE_H
    #ifndef FT_CONFIG_OPTION_USE_PNG
    #error
    #endif
    int main(void) { return 0; }"
    FREETYPE_NEED_PNG)

if(FREETYPE_NEED_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        list(APPEND TTF_LIBRARIES ${PNG_LIBRARIES})
    else()
        message(WARNING "FreeType was compiled with PNG support, "
                        "but no PNG library found.")
    endif()
endif()

#TODO: HarBuzz is another one of those FreeType optional libraries.

set(CMAKE_REQUIRED_INCLUDES)

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
