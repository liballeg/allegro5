include_directories(SYSTEM ${PHYSFS_INCLUDE_DIR})

set(PHYSFS_SOURCES a5_physfs.c a5_physfs_dir.c)

set(PHYSFS_INCLUDE_FILES allegro5/allegro_physfs.h)

# This allows the monolith build to find the files.
set(PHYSFS_INCLUDE_DIRECTORIES ${PHYSFS_INCLUDE_DIR})

set_our_header_properties(${PHYSFS_INCLUDE_FILES})

add_our_addon_library(allegro_physfs
   AllegroPhysfs-${A5O_SOVERSION}
   "${PHYSFS_SOURCES};${PHYSFS_INCLUDE_FILES}"
   "-DA5O_PHYSFS_SRC"
   "${A5O_LINK_WITH};${PHYSFS_LIBRARIES}"
   )

install_our_headers(${PHYSFS_INCLUDE_FILES})

add_addon(physfs)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
