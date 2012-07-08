include_directories(SYSTEM ${PHYSFS_INCLUDE_DIR})

set(PHYSFS_SOURCES a5_physfs.c a5_physfs_dir.c)

set(PHYSFS_INCLUDE_FILES allegro5/allegro_physfs.h)

#set *_INCLUDE_DIRECTORIES for add_addon (so monolith build will find files)
set(PHYSFS_INCLUDE_DIRECTORIES ${PHYSFS_INCLUDE_DIR})

set_our_header_properties(${PHYSFS_INCLUDE_FILES})

add_our_library(allegro_physfs
   "${PHYSFS_SOURCES};${PHYSFS_INCLUDE_FILES}"
   "-DALLEGRO_PHYSFS_SRC"
   "${ALLEGRO_LINK_WITH};${PHYSFS_LIBRARIES}"
   )

set_our_framework_properties(allegro_physfs AllegroPhysfs-${ALLEGRO_SOVERSION})

install_our_library(allegro_physfs)
install_our_headers(${PHYSFS_INCLUDE_FILES})

set(PHYSFS_LINK_WITH allegro_physfs PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
