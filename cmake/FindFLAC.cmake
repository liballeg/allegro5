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

# MSVC built libraries can name them *_static, which is good as it
# distinguishes import libraries from static libraries with the same extension.
find_library(FLAC_LIBRARY NAMES FLAC libFLAC libFLAC_dynamic libFLAC_static)
find_library(OGG_LIBRARY NAMES ogg ogg_static libogg libogg_static)

# Handle the QUIETLY and REQUIRED arguments and set FLAC_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(FLAC DEFAULT_MSG
    FLAC_INCLUDE_DIR OGG_LIBRARY FLAC_LIBRARY)

if(FLAC_FOUND)
  set(FLAC_LIBRARIES ${FLAC_LIBRARY} ${OGG_LIBRARY})
  if(WIN32)
    set(FLAC_LIBRARIES ${FLAC_LIBRARIES} wsock32)
  endif(WIN32)
else(FLAC_FOUND)
  set(FLAC_LIBRARIES)
endif(FLAC_FOUND)

mark_as_advanced(FLAC_INCLUDE_DIR FLAC_LIBRARY)
