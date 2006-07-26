set(DEMO_EXE_PATH "" CACHE INTERNAL "internal")
set(EXECUTABLE_OUTPUT_PATH ${DEMO_EXE_PATH})

set(DEMO_LIB_PATH "" CACHE INTERNAL "internal")
set(LIBRARY_OUTPUT_PATH ${DEMO_LIB_PATH})

#-----------------------------------------------------------------------------#

add_allegro_executable(demo
    animsel.c
    aster.c
    bullet.c
    demo.c
    demodisp.c
    dirty.c
    expl.c
    game.c
    star.c
    title.c
    )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
