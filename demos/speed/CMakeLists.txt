# Prefer local headers to system directories.
include_directories(
    ${CMAKE_SOURCE_DIR}/addons/main
    ${CMAKE_SOURCE_DIR}/addons/audio
    ${CMAKE_SOURCE_DIR}/addons/font
    ${CMAKE_SOURCE_DIR}/addons/primitives
    )

set(SPEED_SRCS
    a4_aux.c
    badguys.c
    bullets.c
    explode.c
    hiscore.c
    main.c
    message.c
    player.c
    sound.c
    title.c
    view.c
    )

if(NOT SUPPORT_AUDIO OR NOT SUPPORT_PRIMITIVES OR NOT SUPPORT_FONT)
    message(STATUS "Not building SPEED")
    return()
endif()

if(COMPILER_GCC)
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -Wno-strict-prototypes -Wno-missing-prototypes")
endif(COMPILER_GCC)

if(APPLE)
    set(DEMO_EXECUTABLE_TYPE MACOSX_BUNDLE)
else(APPLE)
    set(DEMO_EXECUTABLE_TYPE "${EXECUTABLE_TYPE}")
endif(APPLE)

add_executable(speed ${DEMO_EXECUTABLE_TYPE} ${SPEED_SRCS})

fix_executable(speed)

target_link_libraries(speed
    ${ALLEGRO_MAIN_LINK_WITH}
    ${AUDIO_LINK_WITH}
    ${PRIMITIVES_LINK_WITH}
    ${FONT_LINK_WITH}
    ${IMAGE_LINK_WITH}
    )

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(speed PROPERTIES COMPILE_FLAGS "-DALLEGRO_STATICLINK")
endif(NOT BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
