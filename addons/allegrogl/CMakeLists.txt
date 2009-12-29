#-----------------------------------------------------------------------------#
#
# AllegroGL
#

find_package(OpenGL)
if(NOT OPENGL_FOUND)
    return()
endif()
if(UNIX AND NOT APPLE)
    find_package(X11)
    if(NOT X11_FOUND)
	return()
    endif()
    set(GLX 1)
endif()

if(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE)
    set(ALLEGROGL_HAVE_XF86VIDMODE 1)
endif()

find_library(DLOPEN_LIB dl)
mark_as_advanced(DLOPEN_LIB)
if(DLOPEN_LIB)
    set(ALLEGROGL_HAVE_DYNAMIC_LINK 1)
endif()

if(ALLEGRO_WITH_XWINDOWS)
    set(CMAKE_REQUIRED_LIBRARIES ${OPENGL_gl_LIBRARY})
    check_function_exists(glXGetProcAddress GLXGETPROCADDRESS_FOUND)
    if(NOT GLXGETPROCADDRESS_FOUND)
        check_function_exists(glXGetProcAddressARB GLXGETPROCADDRESSARB_FOUND)
        if(GLXGETPROCADDRESSARB_FOUND)
            set(ALLEGROGL_GLXGETPROCADDRESSARB 1)
        else()
            message(FATAL_ERROR "OpenGL does not export symbol glXGetProcAddress[ARB]")
        endif()
    endif()
    set(CMAKE_REQUIRED_LIBRARIES)
endif(ALLEGRO_WITH_XWINDOWS)

# XXX Mesa generic driver support?

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/allegrogl/alleggl_config.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/include/allegrogl/alleggl_config.h
    @ONLY
    )

include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

#-----------------------------------------------------------------------------#

set(ALLEGROGL_SOURCES
    src/alleggl.c
    src/glvtable.c
    src/scorer.c
    src/math.c
    src/aglf.c
    src/fontconv.c
    src/texture.c
    src/glext.c
    src/gui.c
    src/videovtb.c
    )

if(WIN32)
    list(APPEND ALLEGROGL_SOURCES src/win.c)
endif()
if(DJGPP)
    list(APPEND ALLEGROGL_SOURCES src/djgpp.c)
endif()
if(APPLE)
    list(APPEND ALLEGROGL_SOURCES src/macosx.m)
endif()
if(GLX)
    list(APPEND ALLEGROGL_SOURCES src/x.c)
endif()

set(ALLEGROGL_HEADERS
    include/alleggl.h
    include/allegrogl/gl_ext.h
    include/allegrogl/gl_header_detect.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/allegrogl/alleggl_config.h
    include/allegrogl/GLext/glx_ext_alias.h
    include/allegrogl/GLext/glx_ext_api.h
    include/allegrogl/GLext/glx_ext_defs.h
    include/allegrogl/GLext/glx_ext_list.h
    include/allegrogl/GLext/gl_ext_alias.h
    include/allegrogl/GLext/gl_ext_api.h
    include/allegrogl/GLext/gl_ext_defs.h
    include/allegrogl/GLext/gl_ext_list.h
    include/allegrogl/GLext/wgl_ext_alias.h
    include/allegrogl/GLext/wgl_ext_api.h
    include/allegrogl/GLext/wgl_ext_defs.h
    include/allegrogl/GLext/wgl_ext_list.h
    )
set_source_files_properties(
    include/alleggl.h
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Headers
    )
set_source_files_properties(
    include/allegrogl/gl_ext.h
    include/allegrogl/gl_header_detect.h
    ${CMAKE_CURRENT_BINARY_DIR}/include/allegrogl/alleggl_config.h
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Headers/allegrogl
    )
set_source_files_properties(
    include/allegrogl/GLext/glx_ext_alias.h
    include/allegrogl/GLext/glx_ext_api.h
    include/allegrogl/GLext/glx_ext_defs.h
    include/allegrogl/GLext/glx_ext_list.h
    include/allegrogl/GLext/gl_ext_alias.h
    include/allegrogl/GLext/gl_ext_api.h
    include/allegrogl/GLext/gl_ext_defs.h
    include/allegrogl/GLext/gl_ext_list.h
    include/allegrogl/GLext/wgl_ext_alias.h
    include/allegrogl/GLext/wgl_ext_api.h
    include/allegrogl/GLext/wgl_ext_defs.h
    include/allegrogl/GLext/wgl_ext_list.h
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Headers/allegrogl/GLext
    )

# XXX Maybe AllegroGL should be optionally dynamic?
add_our_library(allegrogl ${ADDON_LINKAGE}
    ${ALLEGROGL_SOURCES}
    ${ALLEGROGL_HEADERS}
    )

set_target_properties(allegrogl
    PROPERTIES
    COMPILE_FLAGS -DALLEGRO_GL_SRC_BUILD
    LINK_FLAGS "${ALLEGRO_LINK_FLAGS}"
    OUTPUT_NAME alleggl
    VERSION ${ALLEGRO_VERSION}
    SOVERSION ${ALLEGRO_SOVERSION}
    )

set_our_framework_properties(allegrogl AllegroGL)

target_link_libraries(allegrogl
    allegro
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    )

sanitize_cmake_link_flags(static_link_with
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
    )
set_target_properties(allegrogl
    PROPERTIES
    static_link_with "${static_link_with}"
    )

install_our_library(allegrogl)
if(NOT WANT_FRAMEWORKS)
    install_our_headers(${ALLEGROGL_HEADERS})
endif(NOT WANT_FRAMEWORKS)

set(SUPPORT_ALLEGROGL 1 PARENT_SCOPE)

#-----------------------------------------------------------------------------#
#
# Examples
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examp)

function(add_our_example nm)
    add_executable(${nm}_agl WIN32 examp/${nm}.c)
    set_target_properties(${nm}_agl
        PROPERTIES
        OUTPUT_NAME examp/${nm}
        )
    target_link_libraries(${nm}_agl allegrogl)
endfunction()

if(WANT_EXAMPLES)
    add_our_example(dialog)
    add_our_example(dumbtest)
    add_our_example(exalleg)
    add_our_example(exalpfnt)
    add_our_example(exblend)
    add_our_example(excamera)
    add_our_example(exext)
    add_our_example(exgui)
    add_our_example(exmasked)
    add_our_example(exmipmaps)
    add_our_example(extext)
    add_our_example(extextur)
    add_our_example(fonttest)
    add_our_example(test)
    add_our_example(tex)

    copy_files(copy_allegrogl_example_data
        examp/a1.bmp
        examp/a8.bmp
        examp/a24.tga
        examp/a32.tga
        examp/demofont.dat
        examp/lucidia.dat
        examp/mysha.pcx
        examp/running.dat
        )
endif(WANT_EXAMPLES)

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
