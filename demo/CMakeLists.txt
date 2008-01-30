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

include_directories(include ../addons/font)

add_executable(demo ${DEMO_SRCS})
set_target_properties(demo PROPERTIES
    COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
    LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
    )
target_link_libraries(demo ${LINK_WITH})
target_link_libraries(demo ${FONT_LINK_WITH})

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

