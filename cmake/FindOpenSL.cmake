# - Find OpenSL (actually OpenSLES)
# Find the OpenSLES includes and libraries
#
#  OPENSL_INCLUDE_DIR - where to find dsound.h
#  OPENSL_LIBRARIES   - List of libraries when using dsound.
#  OPENSL_FOUND       - True if dsound found.

if(OPENSL_INCLUDE_DIR)
    # Already in cache, be silent
    set(OPENSL_FIND_QUIETLY TRUE)
endif(OPENSL_INCLUDE_DIR)

find_path(OPENSL_INCLUDE_DIR SLES/OpenSLES.h)

find_library(OPENSL_LIBRARY NAMES OpenSLES)

# Handle the QUIETLY and REQUIRED arguments and set OPENSL_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(OPENSL DEFAULT_MSG
    OPENSL_INCLUDE_DIR OPENSL_LIBRARY)

if(OPENSL_FOUND)
    set(OPENSL_LIBRARIES ${OPENSL_LIBRARY})
else(OPENSL_FOUND)
    set(OPENSL_LIBRARIES)
endif(OPENSL_FOUND)

mark_as_advanced(OPENSL_INCLUDE_DIR OPENSL_LIBRARY)
