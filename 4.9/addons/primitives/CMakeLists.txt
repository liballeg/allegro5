set(PRIMITIVES_SOURCES
    high_primitives.c
    line_soft.c
    tri_soft.c
    prim_directx.c
    prim_opengl.c
    prim_soft.c
    primitives.c
    )
set(PRIMITIVES_INCLUDE_FILES allegro5/a5_primitives.h)

add_our_library(a5_primitives -${ALLEGRO_VERSION}
    "${PRIMITIVES_SOURCES}"
    "-DA5_PRIMITIVES_SRC"
    "${ALLEGRO_LINK_WITH}"
    )

set(PRIMITIVES_LINK_WITH a5_primitives PARENT_SCOPE)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${PRIMITIVES_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
