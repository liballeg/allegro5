if(SUPPORT_FONT AND SUPPORT_AUDIO AND SUPPORT_ACODEC)
else()
    message(STATUS "Not building skater demo")
    return()
endif()

include_directories(
   ${PROJECT_SOURCE_DIR}/addons/acodec
   ${PROJECT_SOURCE_DIR}/addons/audio
   ${PROJECT_SOURCE_DIR}/addons/font
   ${PROJECT_SOURCE_DIR}/addons/image
   ${PROJECT_SOURCE_DIR}/addons/main
   ${PROJECT_SOURCE_DIR}/addons/primitives
   )

set(DEMO_SRCS
   src/anim.c
   src/background_scroller.c
   src/credits.c
   src/fps.c
   src/framework.c
   src/game.c
   src/gamepad.c
   src/global.c
   src/intro.c
   src/keyboard.c
   src/level_alloc.c
   src/level.c
   src/level_file.c
   src/level_state.c
   src/menu_about.c
   src/menu.c
   src/menu_controls.c
   src/menu_graphics.c
   src/menu_main.c
   src/menu_misc.c
   src/menu_options.c
   src/menu_sound.c
   src/menu_success.c
   src/mouse.c
   src/music.c
   src/physics.c
   src/quadtree.c
   src/screenshot.c
   src/skater.c
   src/token.c
   src/transition.c
   src/vcontroller.c
   )

file(GLOB SKATER_GRAPHICS data/graphics/*.png)
file(GLOB SKATER_AUDIO data/audio/*)
file(GLOB SKATER_MENU data/menu/*)
file(GLOB SKATER_LEVEL data/*.txt)

set(SKATER_DATA
    ${SKATER_GRAPHICS}
    ${SKATER_AUDIO}
    ${SKATER_MENU}
    ${SKATER_LEVEL}
    ${SKATER_FONTS}
    )

set(SKATER_REL_DATA)
foreach(data ${SKATER_DATA})
    file(RELATIVE_PATH relpath "${CMAKE_CURRENT_SOURCE_DIR}" "${data}")
    list(APPEND SKATER_REL_DATA "${relpath}")
endforeach(data)

if(ANDROID)
    add_copy_commands(
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/skater.project/app/src/main/assets"
        ASSETS
        "${SKATER_REL_DATA}"
        )
    add_android_app(skater
        "${DEMO_SRCS};${ASSETS}"
        )
    return()
endif()

if(APPLE)
    set(DEMO_EXECUTABLE_TYPE MACOSX_BUNDLE)
else(APPLE)
    set(DEMO_EXECUTABLE_TYPE "${EXECUTABLE_TYPE}")
endif(APPLE)

add_executable(skater
    ${DEMO_EXECUTABLE_TYPE}
    ${DEMO_SRCS}
    ${SKATER_DATA}
    )

fix_executable(skater)

if(WANT_MONOLITH)
   target_link_libraries(skater
     ${A5O_MONOLITH_LINK_WITH}
     )
else(WANT_MONOLITH)
   target_link_libraries(skater
     ${ACODEC_LINK_WITH}
     ${AUDIO_LINK_WITH}
     ${FONT_LINK_WITH}
     ${IMAGE_LINK_WITH}
     ${A5O_MAIN_LINK_WITH}
     ${PRIMITIVES_LINK_WITH}
     )
endif(WANT_MONOLITH)

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(skater PROPERTIES COMPILE_FLAGS "-DA5O_STATICLINK")
endif(NOT BUILD_SHARED_LIBS)

# Mac OS X bundle support.
set_target_properties(skater PROPERTIES
    MACOSX_BUNDLE_COPYRIGHT "Copyright 2010 Allegro Developers"
    #MACOSX_BUNDLE_ICON_FILE "Icon.icns"
    MACOSX_BUNDLE_INFO_STRING "5.1, Copyright 2010 Allegro Developers"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "5.1"
    MACOSX_BUNDLE_LONG_VERSION_STRING "Allegro Skater Demo v5.1"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.liballeg.skater"
    )

set_source_files_properties(${SKATER_GRAPHICS} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data/graphics"
    )
set_source_files_properties(${SKATER_AUDIO} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data/audio"
    )
set_source_files_properties(${SKATER_MENU} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data/menu"
    )
set_source_files_properties(${SKATER_LEVEL} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data"
    )

copy_data_dir_to_build(copy_skater_data
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "${CMAKE_CURRENT_BINARY_DIR}/data"
    )

add_dependencies(skater copy_skater_data copy_example_data)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
