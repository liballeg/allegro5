include_directories(../kcm_audio)

option(WANT_FLAC "Enable FLAC support (acodec)" on)
option(WANT_VORBIS "Enable Ogg Vorbis support (acodec)" on)
option(WANT_SNDFILE "Enable WAV/AIFF/FLAC support (acodec)" on)

set(ACODEC_EXTRA_LIBS)
set(ACODEC_SOURCES acodec.c)

set(SUPPORT_ACODEC 1 PARENT_SCOPE)

if(WANT_FLAC)
    # XXX flac.c doesn't compile with flac 1.1.2
    # missing FLAC__StreamDecoderInitStatus
    find_path(FLAC_INCLUDE_DIR FLAC/stream_decoder.h PATHS ENV INCLUDE)
    find_library(FLAC_LIBRARY NAMES FLAC PATHS ENV LIB)
    mark_as_advanced(FLAC_INCLUDE_DIR FLAC_LIBRARY)

    if(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)
        set(CAN_FLAC 1)
    endif(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)

    if(CAN_FLAC)
        set(SUPPORT_FLAC 1)
        set(ALLEGRO_CFG_ACODEC_FLAC 1)
        include_directories(${FLAC_INCLUDE_DIR})
        list(APPEND ACODEC_EXTRA_LIBS FLAC)
        list(APPEND ACODEC_SOURCES flac.c)
    endif(CAN_FLAC)
endif(WANT_FLAC)

if(WANT_VORBIS)
    # XXX ogg.c doesn't compiler with libvorbis 1.1.2
    # missing OV_CALLBACKS_NOCLOSE
    find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h PATHS ENV INCLUDE)
    find_library(VORBIS_LIBRARY NAMES vorbisfile PATHS ENV LIB)
    mark_as_advanced(VORBIS_INCLUDE_DIR VORBIS_LIBRARY)

    if(VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY)
        set(CAN_VORBIS 1)
    endif(VORBIS_INCLUDE_DIR AND VORBIS_LIBRARY)

    if(CAN_VORBIS)
        set(SUPPORT_VORBIS 1)
        set(ALLEGRO_CFG_ACODEC_VORBIS 1)
        include_directories(${VORBIS_INCLUDE_DIR})
        list(APPEND ACODEC_EXTRA_LIBS vorbisfile vorbis ogg)
        list(APPEND ACODEC_SOURCES ogg.c)
    endif(CAN_VORBIS)
endif(WANT_VORBIS)

if(WANT_SNDFILE)
    find_path(SNDFILE_INCLUDE_DIR sndfile.h PATHS ENV INCLUDE)
    find_library(SNDFILE_LIBRARY NAMES sndfile PATHS ENV LIB)
    mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)

    if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)
        set(CAN_SNDFILE 1)
    endif(SNDFILE_INCLUDE_DIR AND SNDFILE_LIBRARY)

    if(CAN_SNDFILE)
        set(SUPPORT_SNDFILE 1)
        set(ALLEGRO_CFG_ACODEC_SNDFILE 1)
        include_directories(${SNDFILE_INCLUDE_DIR})
        list(APPEND ACODEC_EXTRA_LIBS sndfile)
        list(APPEND ACODEC_SOURCES wav.c)
    endif(CAN_SNDFILE)
endif(WANT_SNDFILE)

configure_file(
    allegro5/internal/aintern_acodec_cfg.h.cmake
    include/allegro5/internal/aintern_acodec_cfg.h
    )

set(ACODEC_INCLUDE_FILES
    allegro5/acodec.h
    )

macro(add_acodec_build nm extra_flags)
    macro(really_add_acodec_build nm2 target libtype more_extra_flags)
        add_library(${target} ${libtype} ${ACODEC_SOURCES})
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${more_extra_flags} ${LIBRARY_CFLAGS} -DA5_ACODEC_SRC"
            OUTPUT_NAME ${nm2}
            )

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
        target_link_libraries(${nm}_shared ${ADDONS_LINK_WITH})
        target_link_libraries(${nm}_shared ${AUDIO_LIB_TO_LINK}_shared)
    endif(SHARED)

    if(STATIC)
        really_add_acodec_build(${nm}_s ${nm}_static STATIC "-DALLEGRO_STATICLINK")
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
    set(ACODEC_LIB_TO_LINK a5_acodec CACHE INTERNAL "internal")
elseif(GRADE_DEBUG)
    set(ACODEC_LIB_TO_LINK a5_acodecd CACHE INTERNAL "internal")
elseif(GRADE_PROFILE)
    set(ACODEC_LIB_TO_LINK a5_acodecp CACHE INTERNAL "internal")
endif(GRADE_STANDARD)

if(STATIC)
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_static CACHE INTERNAL "internal")
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_static PARENT_SCOPE)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} ${AUDIO_LIB_TO_LINK}_static CACHE INTERNAL "internal")
else(STATIC)
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
    set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_shared PARENT_SCOPE)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} ${AUDIO_LIB_TO_LINK}_shared CACHE INTERNAL "internal")
endif(STATIC)

if(SUPPORT_FLAC)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} FLAC)
endif(SUPPORT_FLAC)

if(SUPPORT_VORBIS)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} vorbisfile)
endif(SUPPORT_VORBIS)

if(SUPPORT_SNDFILE)
    set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} sndfile)
endif(SUPPORT_SNDFILE)

example(ex_acodec ${ACODEC_LINK_WITH})
example(ex_acodec_multi ${ACODEC_LINK_WITH})
#disable until the streaming is done
#example(ex_acodec_stream ${ACODEC_LINK_WITH})

#example(ex_acodec_mixer ${ACODEC_LINK_WITH})

#-----------------------------------------------------------------------------#
#
# Install header files.
#

install(FILES ${ACODEC_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
