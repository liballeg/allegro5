option(WANT_ALSA "Enable ALSA digital audio driver (Unix)" on)
option(WANT_OSS "Enable OSS digital audio driver (Unix)" on)
option(WANT_PULSEAUDIO "Enable PulseAudio audio driver (Unix)" on)
option(WANT_OPENAL "Enable OpenAL digital audio driver" on)
option(WANT_OPENSL "Enable OpenSL digital audio driver (Android)" on)
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
    recorder.c
    )

set(AUDIO_INCLUDE_FILES allegro5/allegro_audio.h)

set_our_header_properties(${AUDIO_INCLUDE_FILES})

set(AUDIO_CONFIGURATION_SUMMARY "")

function(audio_summary msg yesno)
    if(${yesno})
        set(yesno "yes")
    else()
        # No's are more important, so shout them.
        set(yesno "NO")
    endif()
    set(AUDIO_CONFIGURATION_SUMMARY "${AUDIO_CONFIGURATION_SUMMARY}${msg}: ${yesno}\n" PARENT_SCOPE)
endfunction()

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

if(ALLEGRO_UNIX)
    audio_summary(" - OSS" SUPPORT_OSS)
endif()

if(WANT_PULSEAUDIO AND ALLEGRO_UNIX)
    pkg_check_modules(PULSEAUDIO libpulse-simple)
    if(PULSEAUDIO_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${PULSEAUDIO_INCLUDE_DIRS})
        run_c_compile_test("
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
    list(APPEND AUDIO_INCLUDE_DIRECTORIES ${PULSEAUDIO_INCLUDE_DIRS})
    list(APPEND AUDIO_LINK_DIRECTORIES ${PULSEAUDIO_LIBRARY_DIRS})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_PULSEAUDIO)

if(ALLEGRO_UNIX)
    audio_summary(" - PulseAudio" SUPPORT_PULSEAUDIO)
endif()

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
    list(APPEND AUDIO_INCLUDE_DIRECTORIES ${ALSA_INCLUDE_DIRS})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_ALSA)

if(ALLEGRO_UNIX)
    audio_summary(" - ALSA" SUPPORT_ALSA)
endif()

if(WANT_DSOUND AND WIN32)
    find_package(DirectX)
    if(DSOUND_FOUND)
        set(SUPPORT_DSOUND 1)
    endif(DSOUND_FOUND)
endif(WANT_DSOUND AND WIN32)

if(SUPPORT_DSOUND)
    set(ALLEGRO_CFG_KCM_DSOUND 1)
    list(APPEND AUDIO_SOURCES dsound.cpp)
    list(APPEND AUDIO_LIBRARIES ${DSOUND_LIBRARIES})
    list(APPEND AUDIO_INCLUDE_DIRECTORIES ${DSOUND_INCLUDE_DIR})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_DSOUND)

if(WIN32)
    audio_summary(" - DirectSound" SUPPORT_DSOUND)
endif()

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

if(MACOSX)
    audio_summary(" - AudioQueue" SUPPORT_AQUEUE)
endif()

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
    list(APPEND AUDIO_INCLUDE_DIRECTORIES ${OPENAL_INCLUDE_DIR})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_OPENAL)

audio_summary(" - OpenAL" SUPPORT_OPENAL)

if(WANT_OPENSL)
    find_package(OpenSL)
    if(OPENSL_FOUND)
        set(SUPPORT_OPENSL 1)
    endif(OPENSL_FOUND)
endif(WANT_OPENSL)

if(SUPPORT_OPENSL)
    set(ALLEGRO_CFG_KCM_OPENSL 1)
    list(APPEND AUDIO_SOURCES opensl.c)
    list(APPEND AUDIO_LIBRARIES ${OPENSL_LIBRARY})
    list(APPEND AUDIO_INCLUDE_DIRECTORIES ${OPENSL_INCLUDE_DIR})
    set(SUPPORT_AUDIO 1)
endif(SUPPORT_OPENSL)

audio_summary(" - OpenSL" SUPPORT_OPENSL)

if(ALLEGRO_SDL)
    set(ALLEGRO_CFG_KCM_SDL 1)
    list(APPEND AUDIO_SOURCES sdl_audio.c)
    list(APPEND AUDIO_LIBRARIES ${SDL2_LIBRARIES} )
    list(APPEND AUDIO_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIR})
    set(SUPPORT_AUDIO 1)
endif(ALLEGRO_SDL)

configure_file(
    allegro5/internal/aintern_audio_cfg.h.cmake
    ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_audio_cfg.h
    )

if(NOT SUPPORT_AUDIO)
    message("WARNING: allegro_audio wanted but no supported backend found")
    return()
endif(NOT SUPPORT_AUDIO)

include_directories(SYSTEM ${AUDIO_INCLUDE_DIRECTORIES})
link_directories(${AUDIO_LINK_DIRECTORIES})
DelayLoad(AUDIO_LIBRARIES AUDIO_LINK_OPTIONS)
add_our_addon_library(allegro_audio
    AllegroAudio-${ALLEGRO_SOVERSION}
    "${AUDIO_SOURCES};${AUDIO_INCLUDE_FILES}"
    "-DALLEGRO_KCM_AUDIO_SRC"
    "${ALLEGRO_LINK_WITH};${AUDIO_LIBRARIES}"
    )
target_link_options(allegro_audio PRIVATE ${AUDIO_LINK_OPTIONS})

install_our_headers(${AUDIO_INCLUDE_FILES})

add_addon(audio)

set(AUDIO_CONFIGURATION_SUMMARY "${AUDIO_CONFIGURATION_SUMMARY}" PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
