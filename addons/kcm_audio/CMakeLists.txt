set(AUDIO_SOURCES
    audio.c
    kcm_mixer.c
    kcm_sample.c
    kcm_stream.c
    kcm_voice.c
    )
set(AUDIO_INCLUDE_FILES allegro5/kcm_audio.h)

# This is a rather cheap way of doing it, but it works in most cases
check_include_file(AL/al.h OPENAL_FOUND)
if(OPENAL_FOUND)
    set(ALLEGRO_CFG_KCM_OPENAL 1)
    list(APPEND AUDIO_SOURCES openal.c)
    list(APPEND AUDIO_LIBRARIES ${OPENAL_LIBRARY})
endif(OPENAL_FOUND)

if(SUPPORT_A5_ALSA)
    set(ALLEGRO_CFG_KCM_ALSA 1)
    list(APPEND AUDIO_SOURCES alsa.c)
    list(APPEND AUDIO_LIBRARIES asound)
endif(SUPPORT_A5_ALSA)


check_include_file(dsound.h HAVE_DSOUND)
if(HAVE_DSOUND)
    set(ALLEGRO_CFG_KCM_DSOUND 1)
    list(APPEND AUDIO_SOURCES dsound.cpp)
    list(APPEND AUDIO_LIBRARIES dsound)
endif(HAVE_DSOUND)

if(OPENAL_FOUND OR SUPPORT_A5_ALSA OR HAVE_DSOUND)
    set(SUPPORT_KCM_AUDIO 1 PARENT_SCOPE)

    configure_file(
        allegro5/internal/aintern_kcm_cfg.h.cmake
        include/allegro5/internal/aintern_kcm_cfg.h
        )

    macro(add_audio_build nm extra_flags)
        macro(really_add_audio_build nm2 target libtype more_extra_flags)
            add_library(${target} ${libtype} ${AUDIO_SOURCES})
            set_target_properties(${target}
                PROPERTIES
                COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DA5_KCM_AUDIO_SRC"
                OUTPUT_NAME ${nm2}
                )

            target_link_libraries(${target} ${AUDIO_LIBRARIES})

            install(TARGETS ${target}
                    DESTINATION lib
                    LIBRARY PERMISSIONS
                        OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ             GROUP_EXECUTE
                        WORLD_READ             WORLD_EXECUTE
                    )
        endmacro(really_add_audio_build)

        if(SHARED)
            really_add_audio_build(${nm} ${nm}_shared SHARED "")
            target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
        endif(SHARED)

        if(STATIC)
            really_add_audio_build(${nm}_s ${nm}_static STATIC "-DALLEGRO_STATICLINK")
        endif(STATIC)

    endmacro(add_audio_build)

    if(GRADE_STANDARD)
        add_audio_build(kcm_audio
            "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
    endif(GRADE_STANDARD)

    if(GRADE_DEBUG)
        add_audio_build(kcm_audiod
            "-DDEBUGMODE -g")
    endif(GRADE_DEBUG)

    if(GRADE_PROFILE)
        add_audio_build(kcm_audiop
            "-pg -O2 -funroll-loops -ffast-math")
    endif(GRADE_PROFILE)

    if(GRADE_STANDARD)
        set(AUDIO_LIB_TO_LINK kcm_audio CACHE INTERNAL "internal")
    elseif(GRADE_DEBUG)
        set(AUDIO_LIB_TO_LINK kcm_audiod CACHE INTERNAL "internal")
    elseif(GRADE_PROFILE)
        set(AUDIO_LIB_TO_LINK kcm_audiop CACHE INTERNAL "internal")
    endif(GRADE_STANDARD)

    if(STATIC)
        set(AUDIO_LINK_WITH ${AUDIO_LIB_TO_LINK}_static CACHE INTERNAL "internal")
        set(AUDIO_LINK_WITH ${AUDIO_LIB_TO_LINK}_static PARENT_SCOPE)
    else(STATIC)
        set(AUDIO_LINK_WITH ${AUDIO_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
        set(AUDIO_LINK_WITH ${AUDIO_LIB_TO_LINK}_shared PARENT_SCOPE)
    endif(STATIC)

    #-----------------------------------------------------------------------------#
    #
    # Install header files.
    #

    install(FILES ${AUDIO_INCLUDE_FILES}
            DESTINATION include/allegro5
            )
else(OPENAL_FOUND OR SUPPORT_A5_ALSA OR HAVE_DSOUND)
    message(STATUS "kcm_audio wanted but no supported backend found\n")
endif(OPENAL_FOUND OR SUPPORT_A5_ALSA OR HAVE_DSOUND)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
