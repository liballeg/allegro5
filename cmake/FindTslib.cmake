# - Find tslib
#
#  TSLIB_INCLUDE_DIR - where to find tslib.h.
#  TSLIB_LIBRARIES   - List of libraries when using tslib.
#  TSLIB_FOUND       - True if tslib found.

if(TSLIB_INCLUDE_DIR)
    # Already in cache, be silent
    set(TSLIB_FIND_QUIETLY TRUE)
endif(TSLIB_INCLUDE_DIR)
find_path(TSLIB_INCLUDE_DIR tslib.h)
find_library(TSLIB_LIBRARY NAMES ts)
# Handle the QUIETLY and REQUIRED arguments and set TSLIB_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TSLIB DEFAULT_MSG
    TSLIB_INCLUDE_DIR TSLIB_LIBRARY)

mark_as_advanced(TSLIB_INCLUDE_DIR)
mark_as_advanced(TSLIB_LIBRARY)
