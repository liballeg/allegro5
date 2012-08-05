set(PRIMITIVES_SOURCES
    high_primitives.c
    line_soft.c
    prim_directx.c
    prim_opengl.c
    prim_soft.c
    point_soft.c
    primitives.c
    directx_shaders.c
    )

if(WIN32)
    # Add this file conditionally.
    # The Debian folks want to remove it because it contains precompiled code.
    list(APPEND PRIMITIVES_SOURCES directx_shaders.c)
endif(WIN32)

set(PRIMITIVES_INCLUDE_FILES allegro5/allegro_primitives.h)

set_our_header_properties(${PRIMITIVES_INCLUDE_FILES})

add_our_library(allegro_primitives
    "${PRIMITIVES_SOURCES};${PRIMITIVES_INCLUDE_FILES}"
    "-DALLEGRO_PRIMITIVES_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set_our_framework_properties(allegro_primitives AllegroPrimitives-${ALLEGRO_SOVERSION})

install_our_library(allegro_primitives)
install_our_headers(${PRIMITIVES_INCLUDE_FILES})

set(PRIMITIVES_LINK_WITH allegro_primitives PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
