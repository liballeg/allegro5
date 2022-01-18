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

find_package(Ogg)
if(OGG_FOUND)
    find_path(THEORA_INCLUDE_DIR theora/theora.h)
    # MSVC built theora may be named *_static
    # The provided project files name the library with the lib prefix.
    find_library(THEORA_LIBRARY NAMES
          theoradec theoradec_static
          libtheoradec libtheoradec_static
          theora theora_static
          libtheora libtheora_static
          )
    # Handle the QUIETLY and REQUIRED arguments and set THEORA_FOUND
    # to TRUE if all listed variables are TRUE.
    include(FindPackageHandleStandardArgs)
    set(FPHSA_NAME_MISMATCHED TRUE)
    find_package_handle_standard_args(THEORA DEFAULT_MSG
          THEORA_INCLUDE_DIR THEORA_LIBRARY)
endif(OGG_FOUND)

if(THEORA_FOUND)
    set(THEORA_LIBRARIES ${THEORA_LIBRARY} ${OGG_LIBRARY})
else(THEORA_FOUND)
  set(THEORA_LIBRARIES)
endif(THEORA_FOUND)

mark_as_advanced(THEORA_INCLUDE_DIR)
mark_as_advanced(THEORA_LIBRARY)

