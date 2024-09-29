set(PRIMITIVES_SOURCES
    high_primitives.c
    line_soft.c
    point_soft.c
    polygon.c
    polyline.c
    prim_directx.cpp
    prim_opengl.c
    prim_soft.c
    prim_util.c
    primitives.c
    triangulator.c
    )

if(WIN32)
    # Add this file conditionally.
    # The Debian folks want to remove it because it contains precompiled code.
    list(APPEND PRIMITIVES_SOURCES directx_shaders.cpp)
endif(WIN32)

set(PRIMITIVES_INCLUDE_FILES allegro5/allegro_primitives.h)

set_our_header_properties(${PRIMITIVES_INCLUDE_FILES})

add_our_addon_library(allegro_primitives
    AllegroPrimitives-${A5O_SOVERSION}
    "${PRIMITIVES_SOURCES};${PRIMITIVES_INCLUDE_FILES}"
    "-DA5O_PRIMITIVES_SRC"
    "${A5O_LINK_WITH}"
    )

install_our_headers(${PRIMITIVES_INCLUDE_FILES})

add_addon(primitives)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
