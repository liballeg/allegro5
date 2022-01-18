# - Find DUMB
# Find the native DUMB includes and libraries
#
#  DUMB_INCLUDE_DIR - where to find DUMB headers.
#  DUMB_LIBRARIES   - List of libraries when using libDUMB.
#  DUMB_FOUND       - True if libDUMB found.

if(DUMB_INCLUDE_DIR)
    # Already in cache, be silent
    set(DUMB_FIND_QUIETLY TRUE)
endif(DUMB_INCLUDE_DIR)

find_path(DUMB_INCLUDE_DIR dumb.h)

find_library(DUMB_LIBRARY NAMES dumb libdumb dumb_static libdumb_static)

# Handle the QUIETLY and REQUIRED arguments and set DUMB_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(DUMB DEFAULT_MSG
    DUMB_INCLUDE_DIR DUMB_LIBRARY)

if(DUMB_FOUND)
  set(DUMB_LIBRARIES ${DUMB_LIBRARY})
  if(NOT MSVC)
    set(DUMB_LIBRARIES ${DUMB_LIBRARIES};m)
  endif()
else(DUMB_FOUND)
  set(DUMB_LIBRARIES)
endif(DUMB_FOUND)

mark_as_advanced(DUMB_INCLUDE_DIR DUMB_LIBRARY)
