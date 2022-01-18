# - Find OpenGLES
# Find the native OpenGLES includes and libraries
#
#  OPENGLES1_INCLUDE_DIR - where to find GLES/gl.h, etc.
#  OPENGLES1_LIBRARIES   - List of libraries when using OpenGLES.
#  OPENGLES1_FOUND       - True if OpenGLES found.

if(OPENGLES1_INCLUDE_DIR)
    # Already in cache, be silent
    set(OPENGLES1_FIND_QUIETLY TRUE)
endif(OPENGLES1_INCLUDE_DIR)

find_path(OPENGLES1_INCLUDE_DIR GLES/gl.h)

find_library(OPENGLES1_gl_LIBRARY NAMES GLESv1_CM)

# Handle the QUIETLY and REQUIRED arguments and set OPENGLES1_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(OPENGLES1 DEFAULT_MSG
    OPENGLES1_INCLUDE_DIR OPENGLES1_gl_LIBRARY)

set(OPENGLES1_LIBRARIES ${OPENGLES1_gl_LIBRARY})

mark_as_advanced(OPENGLES1_INCLUDE_DIR)
mark_as_advanced(OPENGLES1_gl_LIBRARY)
