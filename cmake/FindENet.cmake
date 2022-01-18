# - Find ENet
# Find the native ENet includes and libraries
#
#  ENET_INCLUDE_DIR - where to find ENet headers.
#  ENET_LIBRARIES   - List of libraries when using libenet.
#  ENET_FOUND       - True if libenet found.

if(ENET_INCLUDE_DIR)
    # Already in cache, be silent
    set(ENET_FIND_QUIETLY TRUE)
endif(ENET_INCLUDE_DIR)

find_path(ENET_INCLUDE_DIR enet/enet.h)

find_library(ENET_LIBRARY NAMES enet enet_static libenet libenet_static)

# Handle the QUIETLY and REQUIRED arguments and set ENET_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(ENET DEFAULT_MSG
    ENET_INCLUDE_DIR ENET_LIBRARY)

if(ENET_FOUND)
  set(ENET_LIBRARIES ${ENET_LIBRARY})
else(ENET_FOUND)
  set(ENET_LIBRARIES)
endif(ENET_FOUND)

mark_as_advanced(ENET_INCLUDE_DIR ENET_LIBRARY)
