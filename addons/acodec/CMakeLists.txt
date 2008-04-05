set(ACODEC_SOURCES acodec.c flac.c ex.c ogg.c wav.c)
set(ACODEC_INCLUDE_FILES allegro5/acodec.h)

macro(add_acodec_build nm extra_flags)
    macro(really_add_acodec_build nm target libtype more_extra_flags)
        add_library(${target} ${libtype} ${ACODEC_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm}
            )

        target_link_libraries(${target} ${LINK_WITH})

        install(TARGETS ${target}
                DESTINATION lib
                LIBRARY PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ             GROUP_EXECUTE
                    WORLD_READ             WORLD_EXECUTE
                )
    endmacro(really_add_acodec_build)

    if(SHARED)
        really_add_acodec_build(${nm} ${nm}_shared SHARED "")
    endif(SHARED)

    if(STATIC)
        really_add_acodec_build(${nm} ${nm}_static STATIC "")
    endif(STATIC)

endmacro(add_acodec_build)

if(GRADE_STANDARD)
    add_acodec_build(a5_acodec
        "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
endif(GRADE_STANDARD)

if(GRADE_DEBUG)
    add_acodec_build(a5_acodecd
        "-DDEBUGMODE -g")
endif(GRADE_DEBUG)

if(GRADE_PROFILE)
    add_acodec_build(a5_acodecp
        "-pg -O2 -funroll-loops -ffast-math")
endif(GRADE_PROFILE)


if(GRADE_STANDARD)
    set(ACODEC_LIB_TO_LINK a5_acodec a5_audio CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(ACODEC_LIB_TO_LINK a5_acodecd a5_audiod CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(ACODEC_LIB_TO_LINK a5_acodecp a5_audiop CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_static sndfile FLAC vorbisfile CACHE INTERNAL "internal")
else(STATIC)
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_shared sndfile FLAC vorbisfile CACHE INTERNAL "internal")
endif(STATIC)

macro(add_acodec_example nm)
    add_executable(${nm} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${ACODEC_LINK_WITH})
endmacro(add_acodec_example)

add_acodec_example(ex ex.c)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${ACODEC_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

