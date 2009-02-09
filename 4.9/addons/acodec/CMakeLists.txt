include_directories(../kcm_audio)

option(WANT_FLAC "Enable FLAC support (acodec)" on)
option(WANT_VORBIS "Enable Ogg Vorbis support (acodec)" on)
option(WANT_SNDFILE "Enable WAV/AIFF/FLAC support (acodec)" on)

set(ACODEC_EXTRA_LIBS)
set(ACODEC_SOURCES
    acodec.c
    wav.c
    )

if(SUPPORT_KCM_AUDIO)
    set(SUPPORT_ACODEC 1)
    set(SUPPORT_ACODEC 1 PARENT_SCOPE)
endif(SUPPORT_KCM_AUDIO)

if(WANT_FLAC)
    find_package(FLAC)
    if(FLAC_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${FLAC_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${FLAC_LIBRARIES})
        check_c_source_compiles("
            #include <FLAC/stream_decoder.h>
            int main(void)
            {
                FLAC__StreamDecoderInitStatus status;
                return 0;
            }"
            FLAC_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(FLAC_COMPILES)
            set(ALLEGRO_CFG_ACODEC_FLAC 1)
            include_directories(${FLAC_INCLUDE_DIR})
            list(APPEND ACODEC_EXTRA_LIBS ${FLAC_LIBRARIES})
            list(APPEND ACODEC_SOURCES flac.c)
        endif(FLAC_COMPILES)
    else(FLAC_FOUND)
        message("WARNING: libFLAC not found, disabling support.")
    endif(FLAC_FOUND)
endif(WANT_FLAC)

if(WANT_VORBIS)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${VORBIS_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${VORBIS_LIBRARIES})
        check_c_source_compiles("
            #include <vorbis/vorbisfile.h>
            int main(void)
            {
                OggVorbis_File f;
                ov_callbacks callback;
                vorbis_info *v = 0;
                ov_info(&f, -1);
                callback = OV_CALLBACKS_NOCLOSE;
                return 0;
            }"
            VORBIS_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(VORBIS_COMPILES)
            set(ALLEGRO_CFG_ACODEC_VORBIS 1)
            include_directories(${VORBIS_INCLUDE_DIR})
            list(APPEND ACODEC_EXTRA_LIBS ${VORBIS_LIBRARIES})
            list(APPEND ACODEC_SOURCES ogg.c)
        endif(VORBIS_COMPILES)
    else(VORBIS_FOUND)
        message("WARNING: libvorbis not found, disabling support.")
    endif(VORBIS_FOUND)
endif(WANT_VORBIS)

if(WANT_SNDFILE)
    find_package(Sndfile)
    if(SNDFILE_FOUND)
        set(ALLEGRO_CFG_ACODEC_SNDFILE 1)
        include_directories(${SNDFILE_INCLUDE_DIR})
        list(APPEND ACODEC_EXTRA_LIBS ${SNDFILE_LIBRARIES})
        list(APPEND ACODEC_SOURCES sndfile.c)
    else(SNDFILE_FOUND)
        message("WARNING: libsndfile not found, disabling support.")
    endif(SNDFILE_FOUND)
endif(WANT_SNDFILE)

configure_file(
    allegro5/internal/aintern_acodec_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_acodec_cfg.h
    )

set(ACODEC_INCLUDE_FILES allegro5/acodec.h)

if(SUPPORT_ACODEC)
    add_our_library(a5_acodec -${ALLEGRO_VERSION}
        "${ACODEC_SOURCES}"
        "-DA5_ACODEC_SRC"
        "${AUDIO_LINK_WITH};${ACODEC_EXTRA_LIBS}"
        )

    set(ACODEC_LINK_WITH a5_acodec PARENT_SCOPE)

    install(FILES ${ACODEC_INCLUDE_FILES}
            DESTINATION include/allegro5
            )
endif(SUPPORT_ACODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
