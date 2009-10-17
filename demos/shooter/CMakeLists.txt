include_directories(${CMAKE_CURRENT_SOURCE_DIR})    # for shooter.ico

add_our_executable(shooter WIN32
    animsel.c
    bullet.c
    dirty.c
    expl.c
    star.c
    aster.c
    demo.c
    display.c
    game.c
    title.c
    shooter.rc
    )

copy_files(copy_shooter_data
    demo.dat
    )

#-----------------------------------------------------------------------------#
# vim: set sts=4 sw=4 et:
