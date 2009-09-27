set(PRIMITIVES_SOURCES
    high_primitives.c
    line_soft.c
    tri_soft.c
    prim_directx.c
    prim_opengl.c
    prim_soft.c
    point_soft.c
    primitives.c
    )
set(PRIMITIVES_INCLUDE_FILES allegro5/allegro_primitives.h)

add_our_library(allegro_primitives -${ALLEGRO_VERSION}
    "${PRIMITIVES_SOURCES}"
    "-DALLEGRO_PRIMITIVES_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set(PRIMITIVES_LINK_WITH allegro_primitives PARENT_SCOPE)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${PRIMITIVES_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
