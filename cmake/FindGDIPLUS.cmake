# - Find GDI+
# Find the GDI+ includes and libraries
#
#  GDIPLUS_INCLUDE_DIR - where to find gdiplus.h
#  GDIPLUS_LIBRARIES   - List of libraries when using GDI+.
#  GDIPLUS_FOUND       - True if GDI+ found.

if(GDIPLUS_INCLUDE_DIR)
    # Already in cache, be silent
    set(GDIPLUS_FIND_QUIETLY TRUE)
endif(GDIPLUS_INCLUDE_DIR)

find_path(GDIPLUS_INCLUDE_DIR NAMES GdiPlus.h gdiplus.h)
if(EXISTS ${GDIPLUS_INCLUDE_DIR}/GdiPlus.h)
    set(GDIPLUS_LOWERCASE 0 CACHE INTERNAL "Is GdiPlus.h spelt with lowercase?")
else()
    set(GDIPLUS_LOWERCASE 1 CACHE INTERNAL "Is GdiPlus.h spelt with lowercase?")
endif()

find_library(GDIPLUS_LIBRARY NAMES gdiplus)

# Handle the QUIETLY and REQUIRED arguments and set GDIPLUS_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDIPLUS DEFAULT_MSG
    GDIPLUS_INCLUDE_DIR GDIPLUS_LIBRARY)

if(GDIPLUS_FOUND)
    set(GDIPLUS_LIBRARIES ${GDIPLUS_LIBRARY})
else(GDIPLUS_FOUND)
    set(GDIPLUS_LIBRARIES)
endif(GDIPLUS_FOUND)

mark_as_advanced(GDIPLUS_INCLUDE_DIR GDIPLUS_LIBRARY GDIPLUS_LOWERCASE)
