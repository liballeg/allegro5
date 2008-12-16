set(DEMO_SRCS
    src/Asteroid.cpp
    src/BitmapResource.cpp
    src/Bullet.cpp
    src/ButtonWidget.cpp
    src/Debug.cpp
    src/DisplayResource.cpp
    src/Enemy.cpp
    src/Entity.cpp
    src/Error.cpp
    src/Explosion.cpp
    src/GUI.cpp
    src/GenericResource.cpp
    src/Input.cpp
    src/LargeAsteroid.cpp
    src/LargeBullet.cpp
    src/LargeSlowBullet.cpp
    src/MediumAsteroid.cpp
    src/Misc.cpp
    src/Player.cpp
    src/PowerUp.cpp
    src/Resource.cpp
    src/ResourceManager.cpp
    src/SampleResource.cpp
    src/StreamResource.cpp
    src/SmallAsteroid.cpp
    src/SmallBullet.cpp
    src/UFO.cpp
    src/a5teroids.cpp
    src/collision.cpp
    src/logic.cpp
    src/render.cpp
    src/sound.cpp
    src/wave.cpp
    )

file(GLOB_RECURSE DEMO_GFX data/gfx/*.tga)
file(GLOB_RECURSE DEMO_SFX data/sfx/*.ogg)
set(DEMO_ICON data/gfx/Icon.icns)

if(USE_CXX AND SUPPORT_FONT AND SUPPORT_KCM_AUDIO AND SUPPORT_ACODEC)
    set_source_files_properties(${DEMO_SRCS} PROPERTIES LANGUAGE "CXX")
    include_directories(
        include
        ../addons/font
        ../addons/kcm_audio
        ../addons/acodec
        )

    if(MACOSX)
	set(DEMO_EXECUTABLE_TYPE MACOSX_BUNDLE)
    else(MACOSX)
	set(DEMO_EXECUTABLE_TYPE "${EXECUTABLE_TYPE}")
    endif(MACOSX)

    add_executable(demo ${DEMO_EXECUTABLE_TYPE} ${DEMO_SRCS}
	${DEMO_GFX} ${DEMO_SFX} ${DEMO_ICON})
    set_target_properties(demo PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS} ${EXE_CFLAGS}"
        )

    target_link_libraries(demo
        ${FONT_LINK_WITH}
        ${IIO_LINK_WITH}
        ${ACODEC_LINK_WITH}
        ${AUDIO_LINK_WITH}
        ${LINK_WITH}
        )

    # Mac OS X bundle support.
    set_target_properties(demo PROPERTIES
	MACOSX_BUNDLE_COPYRIGHT "Copyright 2008 Allegro Developers"
	MACOSX_BUNDLE_ICON_FILE "Icon.icns"
	MACOSX_BUNDLE_INFO_STRING "4.9, Copyright 2008 Allegro Developers"
	MACOSX_BUNDLE_SHORT_VERSION_STRING "4.9"
	MACOSX_BUNDLE_LONG_VERSION_STRING "A5teroids v4.9"
	MACOSX_BUNDLE_GUI_IDENTIFIER "org.liballeg.A5teroids"
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

else(USE_CXX AND SUPPORT_FONT AND SUPPORT_KCM_AUDIO AND SUPPORT_ACODEC)
    message(STATUS "Not building demo")
endif(USE_CXX AND SUPPORT_FONT AND SUPPORT_KCM_AUDIO AND SUPPORT_ACODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
