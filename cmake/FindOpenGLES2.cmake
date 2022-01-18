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
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(OPENGLES2 DEFAULT_MSG
    OPENGLES2_INCLUDE_DIR OPENGLES2_gl_LIBRARY OPENGLES2_egl_LIBRARY)

set(OPENGLES2_LIBRARIES ${OPENGLES2_gl_LIBRARY} ${OPENGLES2_egl_LIBRARY})

if(ALLEGRO_RASPBERRYPI)
    if(EXISTS "/opt/vc/lib/libbrcmGLESv2.so")
        set(OPENGLES2_LIBRARIES "/opt/vc/lib/libbrcmGLESv2.so;/opt/vc/lib/libbrcmEGL.so;/opt/vc/lib/libbcm_host.so")
    else()
        set(OPENGLES2_LIBRARIES "/opt/vc/lib/libGLESv2.so;/opt/vc/lib/libEGL.so;/opt/vc/lib/libbcm_host.so")
    endif()
endif(ALLEGRO_RASPBERRYPI)

mark_as_advanced(OPENGLES2_INCLUDE_DIR)
mark_as_advanced(OPENGLES2_gl_LIBRARY)
mark_as_advanced(OPENGLES2_egl_LIBRARY)
