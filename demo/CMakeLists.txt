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

if(USE_CXX AND SUPPORT_FONT AND SUPPORT_KCM_AUDIO AND SUPPORT_ACODEC)
    set_source_files_properties(${DEMO_SRCS} PROPERTIES LANGUAGE "CXX")
    include_directories(
        include
        ../addons/font
        ../addons/kcm_audio
        ../addons/acodec
        )

    add_executable(demo ${EXECUTABLE_TYPE} ${DEMO_SRCS})
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
else(USE_CXX AND SUPPORT_FONT AND SUPPORT_KCM_AUDIO AND SUPPORT_ACODEC)
    message(STATUS "Not building demo")
endif(USE_CXX AND SUPPORT_FONT AND SUPPORT_KCM_AUDIO AND SUPPORT_ACODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

