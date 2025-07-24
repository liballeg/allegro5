set(PRIMITIVES_SOURCES
    high_primitives.c
    polygon.c
    polyline.c
    prim_soft.c
    prim_util.c
    primitives.c
    triangulator.c
    )

set(PRIMITIVES_INCLUDE_FILES allegro5/allegro_primitives.h)

set_our_header_properties(${PRIMITIVES_INCLUDE_FILES})

add_our_addon_library(allegro_primitives
    AllegroPrimitives-${ALLEGRO_SOVERSION}
    "${PRIMITIVES_SOURCES};${PRIMITIVES_INCLUDE_FILES}"
    "-DALLEGRO_PRIMITIVES_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

install_our_headers(${PRIMITIVES_INCLUDE_FILES})

add_addon(primitives)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
