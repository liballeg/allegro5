option(WANT_ALSA "Enable ALSA digital audio driver (Unix)" on)
option(WANT_OSS "Enable OSS digital audio driver (Unix)" on)
option(WANT_PULSEAUDIO "Enable PulseAudio audio driver (Unix)" on)
option(WANT_OPENAL "Enable OpenAL digital audio driver" on)
option(WANT_DSOUND "Enable DSound digital audio driver (Windows)" on)
option(WANT_AQUEUE "Enable AudioQueue digital audio driver (Mac)" on)

set(AUDIO_SOURCES
    audio.c
    audio_io.c
    kcm_dtor.c
    kcm_instance.c
    kcm_mixer.c
    kcm_sample.c
    kcm_stream.c
    kcm_voice.c
    )

set(AUDIO_INCLUDE_FILES allegro5/allegro_audio.h)

set_our_header_properties(${AUDIO_INCLUDE_FILES})

# The platform conditions are not really necessary but prevent confusing the
# user, e.g. it's pretty weird to get a warning about missing DSound on Unix.

if(WANT_OSS AND ALLEGRO_UNIX)
    include(AllegroFindOSS)
    if(OSS_FOUND)
        set(SUPPORT_OSS 1)
    endif(OSS_FOUND)
endif(WANT_OSS AND ALLEGRO_UNIX)

if(SUPPORT_OSS)
    set(ALLEGRO_CFG_KCM_OSS 1)
    list(APPEND AUDIO_SOURCES oss.c)
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_OSS)

if(WANT_PULSEAUDIO AND ALLEGRO_UNIX)
    pkg_check_modules(PULSEAUDIO libpulse-simple)
    if(PULSEAUDIO_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${PULSEAUDIO_INCLUDE_DIRS})
        check_c_source_compiles("
            #include <pulse/simple.h>
            #include <pulse/error.h>
            #include <pulse/introspect.h>
            #include <pulse/mainloop.h>
            int main(void)
            {
                /* Require pulseaudio 0.9.15 */
                pa_context *c;
                pa_sink_info *si;
                pa_sink_state_t *ss;
                return 0;
            }"
            PULSEAUDIO_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(SUPPORT_PULSEAUDIO ${PULSEAUDIO_COMPILES})
        if(NOT SUPPORT_PULSEAUDIO)
            message("WARNING: PulseAudio compile test failed, disabling support")
        endif()
    endif(PULSEAUDIO_FOUND)
endif(WANT_PULSEAUDIO AND ALLEGRO_UNIX)

if(SUPPORT_PULSEAUDIO)
    set(ALLEGRO_CFG_KCM_PULSEAUDIO 1)
    list(APPEND AUDIO_SOURCES pulseaudio.c)
    list(APPEND AUDIO_LIBRARIES ${PULSEAUDIO_LIBRARIES})
    include_directories(SYSTEM ${PULSEAUDIO_INCLUDE_DIRS})
    link_directories(${PULSEAUDIO_LIBRARY_DIRS})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_PULSEAUDIO)

if(WANT_ALSA AND ALLEGRO_UNIX)
    pkg_check_modules(ALSA alsa)
    if(ALSA_FOUND)
        set(SUPPORT_ALSA 1)
    endif(ALSA_FOUND)
endif(WANT_ALSA AND ALLEGRO_UNIX)

if(SUPPORT_ALSA)
    set(ALLEGRO_CFG_KCM_ALSA 1)
    list(APPEND AUDIO_SOURCES alsa.c)
    list(APPEND AUDIO_LIBRARIES ${ALSA_LIBRARIES})
    include_directories(SYSTEM ${ALSA_INCLUDE_DIRS})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_ALSA)

if(WANT_DSOUND AND WIN32)
    find_package(DSound)
    if(DSOUND_FOUND)
        set(SUPPORT_DSOUND 1)
    endif(DSOUND_FOUND)
endif(WANT_DSOUND AND WIN32)

if(SUPPORT_DSOUND)
    set(ALLEGRO_CFG_KCM_DSOUND 1)
    list(APPEND AUDIO_SOURCES dsound.cpp)
    list(APPEND AUDIO_LIBRARIES ${DSOUND_LIBRARIES})
    include_directories(SYSTEM ${DSOUND_INCLUDE_DIR})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_DSOUND)

if(WANT_AQUEUE AND MACOSX)
    # Should check the presence just to be sure.
    find_library(AUDIO_TOOLBOX_LIB NAMES AudioToolbox)
    if(AUDIO_TOOLBOX_LIB)
        find_path(AQUEUE_INCLUDE_DIR AudioToolbox/AudioQueue.h)
        if(AQUEUE_INCLUDE_DIR)
            set(SUPPORT_AQUEUE 1)
        endif(AQUEUE_INCLUDE_DIR)
    endif(AUDIO_TOOLBOX_LIB)

    if(NOT SUPPORT_AQUEUE)
        message("WARNING: could not find AudioQueue. (This is normal on OS X 10.4)")
    endif(NOT SUPPORT_AQUEUE)
endif(WANT_AQUEUE AND MACOSX)

if(SUPPORT_AQUEUE)
    set(ALLEGRO_CFG_KCM_AQUEUE 1)
    list(APPEND AUDIO_SOURCES aqueue.m)
    list(APPEND AUDIO_LIBRARIES ${AUDIO_TOOLBOX_LIB})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_AQUEUE)

if(WANT_OPENAL)
    find_package(OpenAL)
    mark_as_advanced(OPENAL_INCLUDE_DIR OPENAL_LIBRARY)
    if(OPENAL_FOUND)
        set(SUPPORT_OPENAL 1)
    endif(OPENAL_FOUND)
endif(WANT_OPENAL)

if(SUPPORT_OPENAL)
    set(ALLEGRO_CFG_KCM_OPENAL 1)
    list(APPEND AUDIO_SOURCES openal.c)
    list(APPEND AUDIO_LIBRARIES ${OPENAL_LIBRARY})
    include_directories(SYSTEM ${OPENAL_INCLUDE_DIR})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_OPENAL)

configure_file(
    allegro5/internal/aintern_audio_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_audio_cfg.h
    )

if(NOT SUPPORT_AUDIO)
    message("WARNING: allegro_audio wanted but no supported backend found")
    return()
endif(NOT SUPPORT_AUDIO)

# Let examples know that audio is supported.
set(SUPPORT_AUDIO 1 PARENT_SCOPE)

add_our_library(allegro_audio
    "${AUDIO_SOURCES};${AUDIO_INCLUDE_FILES}"
    "-DALLEGRO_KCM_AUDIO_SRC"
    "${ALLEGRO_LINK_WITH};${AUDIO_LIBRARIES}"
    )

set_our_framework_properties(allegro_audio AllegroAudio-${ALLEGRO_SOVERSION})

install_our_library(allegro_audio)
install_our_headers(${AUDIO_INCLUDE_FILES})

set(AUDIO_LINK_WITH allegro_audio PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
