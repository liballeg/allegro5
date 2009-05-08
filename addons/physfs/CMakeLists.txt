
set(PHYSFS_SOURCES a5_physfs.c a5_physfs_dir.c)

set(PHYSFS_INCLUDE_FILES allegro5/a5_physfs.h)

add_our_library(a5_physfs -${ALLEGRO_VERSION}
   "${PHYSFS_SOURCES}"
   "-DA5_PHYSFS_SRC"
   "${ALLEGRO_LINK_WITH};${PHYSFS_LIBRARY}"
   )

set(PHYSFS_LINK_WITH a5_physfs PARENT_SCOPE)

install(FILES ${PHYSFS_INCLUDE_FILES}
        DESTINATION include/allegro5
       )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
