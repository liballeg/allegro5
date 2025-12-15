if(SUPPORT_FONT AND SUPPORT_AUDIO AND SUPPORT_ACODEC)
else()
    message(STATUS "Not building Cosmic Protector")
    return()
endif()

include_directories(
    include
    ${PROJECT_SOURCE_DIR}/addons/main
    ${PROJECT_SOURCE_DIR}/addons/image
    ${PROJECT_SOURCE_DIR}/addons/font
    ${PROJECT_SOURCE_DIR}/addons/audio
    ${PROJECT_SOURCE_DIR}/addons/acodec
    ${PROJECT_SOURCE_DIR}/addons/primitives
    )

set(DEMO_SRCS
    src/Asteroid.cpp
    src/BitmapResource.cpp
    src/Bullet.cpp
    src/ButtonWidget.cpp
    src/Debug.cpp
    src/DisplayResource.cpp
    src/Entity.cpp
    src/Explosion.cpp
    src/FontResource.cpp
    src/Game.cpp
    src/GUI.cpp
    src/Input.cpp
    src/LargeAsteroid.cpp
    src/LargeBullet.cpp
    src/LargeSlowBullet.cpp
    src/MediumAsteroid.cpp
    src/Player.cpp
    src/PowerUp.cpp
    src/Resource.cpp
    src/ResourceManager.cpp
    src/SampleResource.cpp
    src/StreamResource.cpp
    src/SmallAsteroid.cpp
    src/SmallBullet.cpp
    src/UFO.cpp
    src/cosmic_protector.cpp
    src/collision.cpp
    src/joypad_dummy.cpp
    src/logic.cpp
    src/render.cpp
    src/sound.cpp
    src/wave.cpp
    )

if(IPHONE)
    list(APPEND DEMO_SRCS src/cosmic_protector_objc.m)
endif(IPHONE)

set_source_files_properties(${DEMO_SRCS} PROPERTIES LANGUAGE "CXX")

file(GLOB_RECURSE DEMO_GFX RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    data/gfx/*.png data/gfx/*.jpg data/gfx/*.tga)
file(GLOB_RECURSE DEMO_SFX RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    data/sfx/*.ogg)
set(DEMO_ICON data/gfx/Icon.icns)

if(ANDROID)
    add_copy_commands(
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/cosmic_protector.project/app/src/main/assets"
        ASSETS
        "${DEMO_GFX}"
        "${DEMO_SFX}"
        )
    add_android_app(cosmic_protector
        "${DEMO_SRCS};${ASSETS}")
    return()
endif(ANDROID)

if(APPLE)
    set(DEMO_EXECUTABLE_TYPE MACOSX_BUNDLE)
else(APPLE)
    set(DEMO_EXECUTABLE_TYPE "${EXECUTABLE_TYPE}")
endif(APPLE)

add_executable(cosmic_protector
    ${DEMO_EXECUTABLE_TYPE}
    ${DEMO_SRCS}
    ${DEMO_GFX}
    ${DEMO_SFX}
    ${DEMO_ICON}
    )

fix_executable(cosmic_protector)

if(WANT_MONOLITH)
   target_link_libraries(cosmic_protector
       ${ALLEGRO_MONOLITH_LINK_WITH}
       )
else(WANT_MONOLITH)
   target_link_libraries(cosmic_protector
       ${ALLEGRO_MAIN_LINK_WITH}
       ${FONT_LINK_WITH}
       ${IMAGE_LINK_WITH}
       ${AUDIO_LINK_WITH}
       ${ACODEC_LINK_WITH}
       ${PRIMITIVES_LINK_WITH}
       )
endif(WANT_MONOLITH)

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(cosmic_protector PROPERTIES COMPILE_FLAGS "-DALLEGRO_STATICLINK")
endif(NOT BUILD_SHARED_LIBS)

# Mac OS X bundle support.
set_target_properties(cosmic_protector PROPERTIES
    MACOSX_BUNDLE_COPYRIGHT "Copyright 2008 Allegro Developers"
    MACOSX_BUNDLE_ICON_FILE "Icon.icns"
    MACOSX_BUNDLE_INFO_STRING "5.0, Copyright 2008 Allegro Developers"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "5.0"
    MACOSX_BUNDLE_LONG_VERSION_STRING "Cosmic Protector v5.0"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.liballeg.CosmicProtector"
    )
set_source_files_properties(${DEMO_GFX} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data/gfx"
    )
set_source_files_properties(${DEMO_SFX} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/data/sfx"
    )
set_source_files_properties(${DEMO_ICON} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
    )

copy_data_dir_to_build(copy_demo_data
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "${CMAKE_CURRENT_BINARY_DIR}/data"
    )

if(EMSCRIPTEN)
    target_link_options(cosmic_protector PRIVATE "--preload-file" "data")
endif()

add_dependencies(cosmic_protector copy_demo_data)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
