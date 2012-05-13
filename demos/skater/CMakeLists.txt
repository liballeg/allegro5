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
file(GLOB SKATER_FONTS ../../examples/data/*.ttf)

include_directories(
   ${CMAKE_SOURCE_DIR}/addons/acodec
   ${CMAKE_SOURCE_DIR}/addons/audio
   ${CMAKE_SOURCE_DIR}/addons/color
   ${CMAKE_SOURCE_DIR}/addons/font
   ${CMAKE_SOURCE_DIR}/addons/image
   ${CMAKE_SOURCE_DIR}/addons/main
   ${CMAKE_SOURCE_DIR}/addons/memfile
   ${CMAKE_SOURCE_DIR}/addons/native_dialog
   ${CMAKE_SOURCE_DIR}/addons/physfs
   ${CMAKE_SOURCE_DIR}/addons/primitives
   ${CMAKE_SOURCE_DIR}/addons/ttf
   )

if(APPLE)
    set(DEMO_EXECUTABLE_TYPE MACOSX_BUNDLE)
else(APPLE)
    set(DEMO_EXECUTABLE_TYPE "${EXECUTABLE_TYPE}")
endif(APPLE)

if(SUPPORT_FONT AND SUPPORT_TTF AND SUPPORT_AUDIO AND SUPPORT_ACODEC AND SUPPORT_NATIVE_DIALOG)
    add_executable(skater
        ${DEMO_EXECUTABLE_TYPE}
        ${DEMO_SRCS}
        ${SKATER_GRAPHICS}
        ${SKATER_AUDIO}
        ${SKATER_MENU}
        ${SKATER_LEVEL}
        ${SKATER_FONTS}
        )
    
    fix_executable(skater)

    if(WANT_MONOLITH)
       target_link_libraries(skater
         ${ALLEGRO_MONOLITH_LINK_WITH}
         )
    else(WANT_MONOLITH)
       target_link_libraries(skater
         ${ACODEC_LINK_WITH}
         ${AUDIO_LINK_WITH}
         ${COLOR_LINK_WITH}
         ${FONT_LINK_WITH}
         ${IMAGE_LINK_WITH}
         ${ALLEGRO_MAIN_LINK_WITH}
         ${MEMFILE_LINK_WITH}
         ${NATIVE_DIALOG_LINK_WITH}
         ${PHYSFS_LINK_WITH}
         ${PRIMITIVES_LINK_WITH}
         ${TTF_LINK_WITH}
         )
    endif(WANT_MONOLITH)

    if(NOT BUILD_SHARED_LIBS) 
        set_target_properties(skater PROPERTIES COMPILE_FLAGS "-DALLEGRO_STATICLINK")
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
    set_source_files_properties(${SKATER_FONTS} PROPERTIES
	MACOSX_PACKAGE_LOCATION "Resources/data/examples/data"
	)
   
    

else()
    message(STATUS "Not building skater demo")
endif()

copy_data_dir_to_build(copy_skater_data data .)
copy_data_dir_to_build(copy_skater_example_data ../../examples/data data/examples/data)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
