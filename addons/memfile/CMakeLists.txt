
set(MEMFILE_SOURCES memfile.c)

set(MEMFILE_INCLUDE_FILES allegro5/memfile.h)

add_our_library(allegro_memfile -${ALLEGRO_VERSION}
   "${MEMFILE_SOURCES}"
   "-DA5_MEMFILE_SRC"
   "${ALLEGRO_LINK_WITH}"
   )

set(MEMFILE_LINK_WITH allegro_memfile PARENT_SCOPE)

install(FILES ${MEMFILE_INCLUDE_FILES}
        DESTINATION include/allegro5
       )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
