# - Find OpenGLES3
# Find the native OpenGLES3 includes and libraries
# (based on FindOpenGLES2.cmake)
#
#  OPENGLES3_INCLUDE_DIR - where to find GLES3/gl3.h, etc.
#  OPENGLES3_LIBRARIES   - List of libraries when using OpenGLES.
#  OPENGLES3_FOUND       - True if OpenGLES found.

if(OPENGLES3_INCLUDE_DIR)
    # Already in cache, be silent
    set(OPENGLES3_FIND_QUIETLY TRUE)
endif(OPENGLES3_INCLUDE_DIR)

find_path(OPENGLES3_INCLUDE_DIR GLES3/gl3.h)

find_library(OPENGLES3_gl_LIBRARY NAMES GLESv3)
find_library(OPENGLES3_egl_LIBRARY NAMES EGL)

# Handle the QUIETLY and REQUIRED arguments and set OPENGLES3_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(OPENGLES3 DEFAULT_MSG
    OPENGLES3_INCLUDE_DIR OPENGLES3_gl_LIBRARY OPENGLES3_egl_LIBRARY)

set(OPENGLES3_LIBRARIES ${OPENGLES3_gl_LIBRARY} ${OPENGLES3_egl_LIBRARY})

mark_as_advanced(OPENGLES3_INCLUDE_DIR)
mark_as_advanced(OPENGLES3_gl_LIBRARY)
mark_as_advanced(OPENGLES3_egl_LIBRARY)
