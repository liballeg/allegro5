
set(MEMFILE_SOURCES memfile.c)

set(MEMFILE_INCLUDE_FILES allegro5/allegro_memfile.h)

set_our_header_properties(${MEMFILE_INCLUDE_FILES})

add_our_addon_library(allegro_memfile
   AllegroMemfile-${ALLEGRO_SOVERSION}
   "${MEMFILE_SOURCES};${MEMFILE_INCLUDE_FILES}"
   "-DALLEGRO_MEMFILE_SRC"
   "${ALLEGRO_LINK_WITH}"
   )

install_our_headers(${MEMFILE_INCLUDE_FILES})

add_addon(memfile)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
