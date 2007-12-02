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

#-----------------------------------------------------------------------------#
    # Select an Allegro library to link the programs with.  We choose
    # standard, debug, profile in that order, and prefer statically linked
    # over dynamically linked.  It would be worth making this configurable.
    #
if(GRADE_STANDARD)
    set(FONT_LIB_TO_LINK a5_font)
endif(GRADE_STANDARD)
if(NOT FONT_LIB_TO_LINK AND GRADE_DEBUG)
    set(FONT_LIB_TO_LINK a5_fontd)
endif(NOT FONT_LIB_TO_LINK AND GRADE_DEBUG)
if(NOT FONT_LIB_TO_LINK AND GRADE_PROFILE)
    set(FONT_LIB_TO_LINK a5_fontp)
endif(NOT FONT_LIB_TO_LINK AND GRADE_PROFILE)

if(STATIC)
    set(FONT_LINK_WITH ${FONT_LIB_TO_LINK}_static)
    set(MAYBE_FONT_STATICLINK "-DALLEGRO_STATICLINK")
else(STATIC)
    set(FONT_LINK_WITH ${FONT_LIB_TO_LINK}_shared)
endif(STATIC)

include_directories(include ../addons/font)

add_executable(demo ${DEMO_SRCS})
set_target_properties(demo PROPERTIES
    COMPILE_FLAGS "${MAYBE_FONT_STATICLINK}"
    LINK_FLAGS "${MAYBE_FONT_STATICLINK}"
    )
target_link_libraries(demo ${LINK_WITH})
target_link_libraries(demo ${FONT_LINK_WITH})

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

