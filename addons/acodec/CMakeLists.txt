FIND_PATH(FLAC_INCLUDE_DIR FLAC/stream_decoder.h)
FIND_PATH(OGG_INCLUDE_DIR vorbis/vorbisfile.h)
FIND_PATH(SNDFILE_INCLUDE_DIR sndfile.h)

FIND_LIBRARY(FLAC_LIBRARY NAMES FLAC)
FIND_LIBRARY(OGG_LIBRARY NAMES vorbisfile)
FIND_LIBRARY(SNDFILE_LIBRARY NAMES sndfile)

INCLUDE_DIRECTORIES( "../audio" )

IF(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)
    set(FLAC_FOUND TRUE)
    option(WANT_FLAC "Enable FLAC support (acodec)" on)
ENDIF(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)

IF(OGG_INCLUDE_DIR AND OGG_LIBRARY)
    set(OGG_FOUND TRUE)
    option(WANT_OGG "Enable ogg-vorbis support (acodec)" on)
    set(ACODEC_EXTRA_LIBS ${ACODEC_EXTRA_LIBS} vorbisfile vorbis ogg)
ENDIF(OGG_INCLUDE_DIR AND OGG_LIBRARY)

IF(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
    set(SNDFILE_FOUND TRUE)
    option(WANT_SNDFILE "Enable wav/aiff support (acodec)" on)
ENDIF(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)

set(ACODEC_SOURCES acodec.c ex_acodec.c)
set(ACODEC_INCLUDE_FILES allegro5/acodec.h)

if(WANT_FLAC)
    set(ACODEC_SOURCES ${ACODEC_SOURCES} flac.c)
    INCLUDE_DIRECTORIES( ${FLAC_INCLUDE_DIR} )
    set(ALLEGRO_CFG_ACODEC_FLAC 1)
endif(WANT_FLAC)

if(WANT_OGG)
    set(ACODEC_SOURCES ${ACODEC_SOURCES} ogg.c)
    INCLUDE_DIRECTORIES( ${OGG_INCLUDE_DIR} )
    set(ALLEGRO_CFG_ACODEC_OGG 1)
endif(WANT_OGG)

if(WANT_SNDFILE)
    set(ACODEC_SOURCES ${ACODEC_SOURCES} wav.c)
    INCLUDE_DIRECTORIES( ${SNDFILE_INCLUDE_DIR} )
    set(ALLEGRO_CFG_ACODEC_SNDFILE 1)
endif(WANT_SNDFILE)

configure_file(
    allegro5/internal/aintern_acodec.h.cmake
    include/allegro5/internal/aintern_acodec.h
    )

macro(add_acodec_build nm extra_flags)
    macro(really_add_acodec_build nm target libtype more_extra_flags)
        add_library(${target} ${libtype} ${ACODEC_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS}"
            OUTPUT_NAME ${nm}
            )

        target_link_libraries(${target} ${LINK_WITH})
        target_link_libraries(${target} ${AUDIO_LINK_WITH})
        target_link_libraries(${target} ${ACODEC_EXTRA_LIBS})

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
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

if(WANT_FLAC)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} FLAC)
endif(WANT_FLAC)

if(WANT_OGG)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} vorbisfile)
endif(WANT_OGG)

if(WANT_SNDFILE)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} sndfile)
endif(WANT_SNDFILE)

macro(add_acodec_example nm)
    add_executable(${nm} ${EXECUTABLE_TYPE} ${ARGN})
    set_target_properties(${nm} PROPERTIES
        COMPILE_FLAGS "${MAYBE_ALLEGRO_STATICLINK} ${WFLAGS}"
        LINK_FLAGS "${MAYBE_ALLEGRO_STATICLINK}"
        )
    target_link_libraries(${nm} ${LINK_WITH})
    target_link_libraries(${nm} ${ACODEC_LINK_WITH})
endmacro(add_acodec_example)

add_acodec_example(ex_acodec ex_acodec.c)

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${ACODEC_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:

