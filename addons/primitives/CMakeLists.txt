set(PRIMITIVES_SOURCES
    high_primitives.c
    line_soft.c
    tri_soft.c
    prim_directx.c
    prim_opengl.c
    prim_soft.c
    point_soft.c
    primitives.c
    directx_shaders.cpp
    )

set(PRIMITIVES_INCLUDE_FILES allegro5/allegro_primitives.h)

set_our_header_properties(${PRIMITIVES_INCLUDE_FILES})

if(WIN32)
   find_library(D3DX9 d3dx9)
   mark_as_advanced(D3DX9)
   if(NOT D3DX9)
      message(FATAL_ERROR
         "Primitives addon requires the D3DX9 library.")
   endif(NOT D3DX9)
else(WIN32)
   set(D3DX9 "")
endif(WIN32)

add_our_library(allegro_primitives
    "${PRIMITIVES_SOURCES};${PRIMITIVES_INCLUDE_FILES}"
    "-DALLEGRO_PRIMITIVES_SRC"
    "${ALLEGRO_LINK_WITH};${D3DX9}"
    )

set_our_framework_properties(allegro_primitives AllegroPrimitives-${ALLEGRO_SOVERSION})

install_our_library(allegro_primitives)
install_our_headers(${PRIMITIVES_INCLUDE_FILES})

set(PRIMITIVES_LINK_WITH allegro_primitives PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
