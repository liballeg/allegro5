# - Find FLAC
# Find the native FLAC includes and libraries
#
#  FLAC_INCLUDE_DIR - where to find FLAC headers.
#  FLAC_LIBRARIES   - List of libraries when using libFLAC.
#  FLAC_FOUND       - True if libFLAC found.

if(FLAC_INCLUDE_DIR)
    # Already in cache, be silent
    set(FLAC_FIND_QUIETLY TRUE)
endif(FLAC_INCLUDE_DIR)

find_path(FLAC_INCLUDE_DIR FLAC/stream_decoder.h)

find_library(FLAC_LIBRARY NAMES FLAC)

# Handle the QUIETLY and REQUIRED arguments and set FLAC_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLAC DEFAULT_MSG
    FLAC_INCLUDE_DIR FLAC_LIBRARY)

if(FLAC_FOUND)
  set(FLAC_LIBRARIES ${FLAC_LIBRARY})
  if(WIN32)
    set(FLAC_LIBRARIES ${FLAC_LIBRARIES} wsock32)
  endif(WIN32)
else(FLAC_FOUND)
  set(FLAC_LIBRARIES)
endif(FLAC_FOUND)

mark_as_advanced(FLAC_INCLUDE_DIR FLAC_LIBRARY)
