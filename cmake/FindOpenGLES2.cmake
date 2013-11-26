# - Find OpenGLES2
# Find the native OpenGLES2 includes and libraries
#
#  OPENGLES2_INCLUDE_DIR - where to find GLES2/gl.h, etc.
#  OPENGLES2_LIBRARIES   - List of libraries when using OpenGLES.
#  OPENGLES2_FOUND       - True if OpenGLES found.

if(OPENGLES2_INCLUDE_DIR)
    # Already in cache, be silent
    set(OPENGLES2_FIND_QUIETLY TRUE)
endif(OPENGLES2_INCLUDE_DIR)

find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h)

find_library(OPENGLES2_gl_LIBRARY NAMES GLESv2)
find_library(OPENGLES2_egl_LIBRARY NAMES EGL)

# Handle the QUIETLY and REQUIRED arguments and set OPENGLES2_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OPENGLES2 DEFAULT_MSG
    OPENGLES2_INCLUDE_DIR OPENGLES2_gl_LIBRARY OPENGLES2_egl_LIBRARY)

set(OPENGLES2_LIBRARIES ${OPENGLES2_gl_LIBRARY} ${OPENGLES2_egl_LIBRARY})

mark_as_advanced(OPENGLES2_INCLUDE_DIR)
mark_as_advanced(OPENGLES2_gl_LIBRARY)
mark_as_advanced(OPENGLES2_egl_LIBRARY)
