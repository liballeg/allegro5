# - Find opus
# Find the native opus includes and libraries
#
#  OPUS_INCLUDE_DIR - where to find opus.h, etc.
#  OPUS_LIBRARIES   - List of libraries when using opus(file).
#  OPUS_FOUND       - True if opus found.

if(OPUS_INCLUDE_DIR)
    # Already in cache, be silent
    set(OPUS_FIND_QUIETLY TRUE)
endif(OPUS_INCLUDE_DIR)

find_package(Ogg)
if(OGG_FOUND)
        find_path(OPUS_INCLUDE_DIR opusfile.h PATH_SUFFIXES opus)
        # MSVC built opus may be named opus_static
	# The provided project files name the library with the lib prefix.
        find_library(OPUS_LIBRARY
            NAMES opus opus_static libopus libopus_static)
        find_library(OPUSFILE_LIBRARY
            NAMES opusfile opusfile_static libopusfile libopusfile_static)
        # Handle the QUIETLY and REQUIRED arguments and set OPUS_FOUND
	# to TRUE if all listed variables are TRUE.
	include(FindPackageHandleStandardArgs)
        set(FPHSA_NAME_MISMATCHED TRUE)
        find_package_handle_standard_args(OPUS DEFAULT_MSG
            OPUS_INCLUDE_DIR
            OPUS_LIBRARY OPUSFILE_LIBRARY)
endif(OGG_FOUND)

if(OPUS_FOUND)
  set(OPUS_LIBRARIES ${OPUSFILE_LIBRARY} ${OPUS_LIBRARY}
        ${OGG_LIBRARY})
else(OPUS_FOUND)
  set(OPUS_LIBRARIES)
endif(OPUS_FOUND)

mark_as_advanced(OPUS_INCLUDE_DIR)
mark_as_advanced(OPUS_LIBRARY OPUSFILE_LIBRARY)
