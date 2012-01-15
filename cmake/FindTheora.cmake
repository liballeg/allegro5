# - Find theora
# Find the native theora includes and libraries
#
#  THEORA_INCLUDE_DIR - where to find theora.h, etc.
#  THEORA_LIBRARIES   - List of libraries when using theora.
#  THEORA_FOUND       - True if theora found.

if(THEORA_INCLUDE_DIR)
    # Already in cache, be silent
    set(THEORA_FIND_QUIETLY TRUE)
endif(THEORA_INCLUDE_DIR)
find_path(OGG_INCLUDE_DIR ogg/ogg.h)
find_path(THEORA_INCLUDE_DIR theora/theora.h)
# MSVC built ogg/theora may be named ogg_static and theora_static
# The provided project files name the library with the lib prefix.
find_library(OGG_LIBRARY
    NAMES ogg ogg_static libogg libogg_static)
find_library(THEORA_LIBRARY
    NAMES theoradec theoradec_static libtheoradec libtheoradec_static)
# Handle the QUIETLY and REQUIRED arguments and set THEORA_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(THEORA DEFAULT_MSG
    OGG_INCLUDE_DIR THEORA_INCLUDE_DIR
    OGG_LIBRARY THEORA_LIBRARY)

if(THEORA_FOUND)
    set(THEORA_LIBRARIES ${THEORA_LIBRARY} ${OGG_LIBRARY})
else(THEORA_FOUND)
  set(THEORA_LIBRARIES)
endif(THEORA_FOUND)

mark_as_advanced(OGG_INCLUDE_DIR THEORA_INCLUDE_DIR)
mark_as_advanced(OGG_LIBRARY THEORA_LIBRARY)

