# Prefer local headers to system directories.
include_directories(
    ${CMAKE_SOURCE_DIR}/addons/main
    ${CMAKE_SOURCE_DIR}/addons/audio
    ${CMAKE_SOURCE_DIR}/addons/font
    ${CMAKE_SOURCE_DIR}/addons/image
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

set(A4_FONT ${CMAKE_CURRENT_BINARY_DIR}/a4_font.tga)

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

add_executable(speed ${DEMO_EXECUTABLE_TYPE} ${SPEED_SRCS} ${A4_FONT})

fix_executable(speed)

if(WANT_MONOLITH)
   target_link_libraries(speed
       ${ALLEGRO_MONOLITH_LINK_WITH}
       )
else(WANT_MONOLITH)
   target_link_libraries(speed
       ${ALLEGRO_MAIN_LINK_WITH}
       ${AUDIO_LINK_WITH}
       ${PRIMITIVES_LINK_WITH}
       ${FONT_LINK_WITH}
       ${IMAGE_LINK_WITH}
       )
endif(WANT_MONOLITH)

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(speed PROPERTIES COMPILE_FLAGS "-DALLEGRO_STATICLINK")
endif(NOT BUILD_SHARED_LIBS)

set_source_files_properties(${A4_FONT} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

add_custom_command(
    OUTPUT ${A4_FONT}
    COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_SOURCE_DIR}/examples/data/a4_font.tga" ${A4_FONT}
    )

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
