
set(MEMFILE_SOURCES memfile.c)

set(MEMFILE_INCLUDE_FILES allegro5/allegro_memfile.h)

set_our_header_properties(${MEMFILE_INCLUDE_FILES})

add_our_library(allegro_memfile
   "${MEMFILE_SOURCES};${MEMFILE_INCLUDE_FILES}"
   "-DALLEGRO_MEMFILE_SRC"
   "${ALLEGRO_LINK_WITH}"
   )

set_our_framework_properties(allegro_memfile AllegroMemfile-${ALLEGRO_SOVERSION})

install_our_library(allegro_memfile)
install_our_headers(${MEMFILE_INCLUDE_FILES})

set(MEMFILE_LINK_WITH allegro_memfile PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
