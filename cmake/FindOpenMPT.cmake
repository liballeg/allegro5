# - Find OPENMPT
# Find the native OpenMPT includes and libraries
#
#  OPENMPT_INCLUDE_DIR - where to find OpenMPT headers.
#  OPENMPT_LIBRARIES   - List of libraries when using OpenMPT.
#  OPENMPT_FOUND       - True if OpenMPT found.

if(OPENMPT_INCLUDE_DIR)
    # Already in cache, be silent
    set(OPENMPT_FIND_QUIETLY TRUE)
endif(OPENMPT_INCLUDE_DIR)

find_path(OPENMPT_INCLUDE_DIR libopenmpt/libopenmpt.h)

find_library(OPENMPT_LIBRARY NAMES openmpt)

# Handle the QUIETLY and REQUIRED arguments and set OPENMPT_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(OPENMPT DEFAULT_MSG
    OPENMPT_INCLUDE_DIR OGG_LIBRARY OPENMPT_LIBRARY)

if(OPENMPT_FOUND)
  set(OPENMPT_LIBRARIES ${OPENMPT_LIBRARY})
else(OPENMPT_FOUND)
  set(OPENMPT_LIBRARIES)
endif(OPENMPT_FOUND)

mark_as_advanced(OPENMPT_INCLUDE_DIR OPENMPT_LIBRARY)
