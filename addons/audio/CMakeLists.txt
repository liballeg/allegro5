IF(WIN32)
    set(AUDIO_SOURCES audio.c openal.c)
ELSE(WIN32)
    if(APPLE)
        set(AUDIO_SOURCES audio.c openal.c)
    ELSE(APPLE)
        #LINUX needs ALSA
        set(AUDIO_SOURCES audio.c openal.c alsa.c)
    ENDIF(APPLE)
ENDIF(WIN32)

set(AUDIO_INCLUDE_FILES allegro5/audio.h)

macro(add_audio_build nm extra_flags)
    macro(really_add_audio_build nm target libtype more_extra_flags)
        add_library(${target} ${libtype} ${AUDIO_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm}
            )

        target_link_libraries(${target} ${LINK_WITH} openal esd)

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
    endif(SHARED)

    if(STATIC)
        really_add_audio_build(${nm} ${nm}_static STATIC "")
    endif(STATIC)

endmacro(add_audio_build)

if(GRADE_STANDARD)
    add_audio_build(a5_audio
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_audio_build(a5_audiod
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_audio_build(a5_audiop
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)

#note: esd is to satisfy linux 
if(GRADE_STANDARD)
    set(AUDIO_LIB_TO_LINK a5_audio CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(AUDIO_LIB_TO_LINK a5_audiod CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(AUDIO_LIB_TO_LINK a5_audiop CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(AUDIO_LINK_WITH ${AUDIO_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(AUDIO_LINK_WITH ${AUDIO_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

macro(add_audio_example nm)
    add_executable(${nm} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${AUDIO_LINK_WITH})
endmacro(add_audio_example)

#add_audio_example(ex ex.c)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${AUDIO_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

