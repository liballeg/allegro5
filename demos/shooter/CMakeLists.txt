if(SUPPORT_FONT AND SUPPORT_TTF AND SUPPORT_AUDIO AND SUPPORT_ACODEC)
else()
    message(STATUS "Not building scooter demo")
    return()
endif()

include_directories(
   ${PROJECT_SOURCE_DIR}/addons/acodec
   ${PROJECT_SOURCE_DIR}/addons/audio
   ${PROJECT_SOURCE_DIR}/addons/font
   ${PROJECT_SOURCE_DIR}/addons/image
   ${PROJECT_SOURCE_DIR}/addons/main
   ${PROJECT_SOURCE_DIR}/addons/primitives
   ${PROJECT_SOURCE_DIR}/addons/ttf
   )

set(DEMO_SRCS
   aster.c
   bullet.c
   data.c
   demo.c
   expl.c
   game.c
   star.c
   title.c
   ../speed/a4_aux.c
   )

file(COPY ${PROJECT_SOURCE_DIR}/examples/data/DejaVuSans.ttf
        DESTINATION ${CMAKE_BINARY_DIR}/demos/shooter/data)
file(COPY ${PROJECT_SOURCE_DIR}/demos/skater/data/menu/intro_music.ogg
        DESTINATION ${CMAKE_BINARY_DIR}/demos/shooter/data)
file(COPY ${PROJECT_SOURCE_DIR}/demos/skater/data/menu/menu_music.ogg
        DESTINATION ${CMAKE_BINARY_DIR}/demos/shooter/data)
file(RENAME ${CMAKE_BINARY_DIR}/demos/shooter/data/intro_music.ogg
    ${CMAKE_BINARY_DIR}/demos/shooter/data/INTRO_MUSIC.ogg)
file(RENAME ${CMAKE_BINARY_DIR}/demos/shooter/data/menu_music.ogg
    ${CMAKE_BINARY_DIR}/demos/shooter/data/TITLE_MUSIC.ogg)


file(GLOB SHOOTER_DEMO_DATA data/*)

set(SHOOTER_DATA
    ${SHOOTER_DEMO_DATA}
    )
list(APPEND SHOOTER_DATA ${CMAKE_BINARY_DIR}/demos/shooter/data/DejaVuSans.ttf)
list(APPEND SHOOTER_DATA ${CMAKE_BINARY_DIR}/demos/shooter/data/INTRO_MUSIC.ogg)
list(APPEND SHOOTER_DATA ${CMAKE_BINARY_DIR}/demos/shooter/data/TITLE_MUSIC.ogg)


set(SHOOTER_REL_DATA)
foreach(data ${SHOOTER_DATA})
    file(RELATIVE_PATH relpath "${CMAKE_CURRENT_SOURCE_DIR}" "${data}")
    list(APPEND SHOOTER_REL_DATA "${relpath}")
endforeach(data)

if(ANDROID)
    add_copy_commands(
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/shooter.project/app/src/main/assets"
        ASSETS
        "${SHOOTER_REL_DATA}"
        )
    add_android_app(shooter
        "${DEMO_SRCS};${ASSETS}"
        )
    return()
endif()

if(APPLE)
    set(DEMO_EXECUTABLE_TYPE MACOSX_BUNDLE)
else(APPLE)
    set(DEMO_EXECUTABLE_TYPE "${EXECUTABLE_TYPE}")
endif(APPLE)

add_executable(shooter
    ${DEMO_EXECUTABLE_TYPE}
    ${DEMO_SRCS}
    ${SHOOTER_DATA}
    )

fix_executable(shooter)

if(WANT_MONOLITH)
   target_link_libraries(shooter
     ${ALLEGRO_MONOLITH_LINK_WITH}
     )
else(WANT_MONOLITH)
   target_link_libraries(shooter
     ${ACODEC_LINK_WITH}
     ${AUDIO_LINK_WITH}
     ${FONT_LINK_WITH}
     ${IMAGE_LINK_WITH}
     ${ALLEGRO_MAIN_LINK_WITH}
     ${PRIMITIVES_LINK_WITH}
     ${TTF_LINK_WITH}
     )
endif(WANT_MONOLITH)

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(shooter PROPERTIES COMPILE_FLAGS "-DALLEGRO_STATICLINK")
endif(NOT BUILD_SHARED_LIBS)

# Mac OS X bundle support.
set_target_properties(shooter PROPERTIES
    MACOSX_BUNDLE_COPYRIGHT "Copyright 2010 Allegro Developers"
    #MACOSX_BUNDLE_ICON_FILE "Icon.icns"
    MACOSX_BUNDLE_INFO_STRING "5.1, Copyright 2010 Allegro Developers"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "5.1"
    MACOSX_BUNDLE_LONG_VERSION_STRING "Allegro Shooter Demo v5.1"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.liballeg.shooter"
    )

set_source_files_properties(${SHOOTER_DATA} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data"
    )

copy_data_dir_to_build(copy_shooter_data
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "${CMAKE_CURRENT_BINARY_DIR}/data"
    )

add_dependencies(shooter copy_shooter_data copy_example_data)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
