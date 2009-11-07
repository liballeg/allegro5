include_directories(${CMAKE_SOURCE_DIR}/addons/allegrogl/include)
include_directories(${CMAKE_BINARY_DIR}/addons/allegrogl/include)

set(SKATER_SOURCES
    source/anim.c
    source/backscrl.c
    source/credits.c
    source/dblbuf.c
    source/fps.c
    source/framewk.c
    source/game_sk.c
    source/gamepad.c
    source/global.c
    source/intro.c
    source/kbd_sk.c
    source/level.c
    source/lvlalloc.c
    source/lvlfile.c
    source/lvlstate.c
    source/menu.c
    source/menuabt.c
    source/menuctl.c
    source/menugfx.c
    source/menumain.c
    source/menumisc.c
    source/menuopts.c
    source/menusnd.c
    source/menusucc.c
    source/music.c
    source/oglflip.c
    source/pageflip.c
    source/physics.c
    source/quadtree.c
    source/scrshot.c
    source/skater.c
    source/token.c
    source/transitn.c
    source/tribuf.c
    source/virtctl.c
    )

add_our_executable(skater WIN32 ${SKATER_SOURCES})

if(SUPPORT_ALLEGROGL)
    add_our_executable(skater_agl WIN32 ${SKATER_SOURCES})
    set_target_properties(skater_agl
        PROPERTIES
        COMPILE_FLAGS -DDEMO_USE_ALLEGRO_GL
        )
    target_link_libraries(skater_agl allegrogl)
endif(SUPPORT_ALLEGROGL)

copy_files(copy_skater_data
    demo.dat
    level.dat
    )

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
