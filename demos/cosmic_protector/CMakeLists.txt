if(SUPPORT_FONT AND SUPPORT_AUDIO AND SUPPORT_ACODEC)
else()
    message(STATUS "Not building Cosmic Protector")
    return()
endif()

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
    src/logic.cpp
    src/render.cpp
    src/sound.cpp
    src/wave.cpp
    )

set_source_files_properties(${DEMO_SRCS} PROPERTIES LANGUAGE "CXX")

file(GLOB_RECURSE DEMO_GFX data/gfx/*.tga)
file(GLOB_RECURSE DEMO_SFX data/sfx/*.ogg)
set(DEMO_ICON data/gfx/Icon.icns)

include_directories(
    include
    ${CMAKE_SOURCE_DIR}/addons/main
    ${CMAKE_SOURCE_DIR}/addons/image
    ${CMAKE_SOURCE_DIR}/addons/font
    ${CMAKE_SOURCE_DIR}/addons/audio
    ${CMAKE_SOURCE_DIR}/addons/acodec
    )

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

target_link_libraries(cosmic_protector
    ${ALLEGRO_MAIN_LINK_WITH}
    ${FONT_LINK_WITH}
    ${IMAGE_LINK_WITH}
    ${AUDIO_LINK_WITH}
    ${ACODEC_LINK_WITH}
    )

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

copy_data_dir_to_build(copy_demo_data data .)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
