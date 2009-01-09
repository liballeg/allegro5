set(AUDIO_SOURCES
    audio.c
    kcm_dtor.c
    kcm_mixer.c
    kcm_sample.c
    kcm_simple.c
    kcm_stream.c
    kcm_voice.c
    )

set(AUDIO_INCLUDE_FILES allegro5/kcm_audio.h)

if(SUPPORT_OSS)
   set(ALLEGRO_CFG_KCM_OSS 1)
   list(APPEND AUDIO_SOURCES oss.c)
endif(SUPPORT_OSS)

if(SUPPORT_OPENAL)
    set(ALLEGRO_CFG_KCM_OPENAL 1)
    list(APPEND AUDIO_SOURCES openal.c)
    list(APPEND AUDIO_LIBRARIES ${OPENAL_LIBRARY})
    include_directories(${OPENAL_INCLUDE_DIR})
endif(SUPPORT_OPENAL)

if(SUPPORT_ALSA)
    set(ALLEGRO_CFG_KCM_ALSA 1)
    list(APPEND AUDIO_SOURCES alsa.c)
    list(APPEND AUDIO_LIBRARIES ${ALSA_LIBRARIES})
    include_directories(${ALSA_INCLUDE_DIRS})
endif(SUPPORT_ALSA)

if(SUPPORT_DSOUND)
    set(ALLEGRO_CFG_KCM_DSOUND 1)
    list(APPEND AUDIO_SOURCES dsound.cpp)
    list(APPEND AUDIO_LIBRARIES ${DSOUND_LIBRARIES})
    if(COMPILER_GCC)
        list(APPEND AUDIO_LIBRARIES)
    endif(COMPILER_GCC)
    include_directories(${DSOUND_INCLUDE_DIR})
endif(SUPPORT_DSOUND)

configure_file(
    allegro5/internal/aintern_kcm_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_kcm_cfg.h
    )

if(SUPPORT_OSS OR SUPPORT_OPENAL OR SUPPORT_ALSA OR SUPPORT_DSOUND)
    # Let examples know that audio is supported.
    set(SUPPORT_KCM_AUDIO 1 PARENT_SCOPE)

    add_our_library(kcm_audio -${ALLEGRO_VERSION}
        "${AUDIO_SOURCES}"
        "-DA5_KCM_AUDIO_SRC"
        "${ALLEGRO_LINK_WITH};${AUDIO_LIBRARIES}"
        )

    set(AUDIO_LINK_WITH kcm_audio PARENT_SCOPE)

    install(FILES ${AUDIO_INCLUDE_FILES}
            DESTINATION include/allegro5
            )

else(SUPPORT_OSS OR SUPPORT_OPENAL OR SUPPORT_ALSA OR SUPPORT_DSOUND)
    message("WARNING: kcm_audio wanted but no supported backend found")
endif(SUPPORT_OSS OR SUPPORT_OPENAL OR SUPPORT_ALSA OR SUPPORT_DSOUND)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
