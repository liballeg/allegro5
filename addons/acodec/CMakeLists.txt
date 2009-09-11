include_directories(../audio)

option(WANT_FLAC "Enable FLAC support" on)
option(WANT_VORBIS "Enable Ogg Vorbis support" on)

#-----------------------------------------------------------------------------#

set(A5_FLAC_SOURCES flac.c)

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
            set(SUPPORT_FLAC 1)
            set(SUPPORT_FLAC 1 PARENT_SCOPE)
            include_directories(${FLAC_INCLUDE_DIR})
        endif(FLAC_COMPILES)
    else(FLAC_FOUND)
        message("WARNING: libFLAC not found, disabling support.")
    endif(FLAC_FOUND)
endif(WANT_FLAC)

if(SUPPORT_FLAC)
    add_our_library(allegro_flac -${ALLEGRO_VERSION}
        "${A5_FLAC_SOURCES}"
        "-DALLEGRO_FLAC_SRC"
        "${AUDIO_LINK_WITH};${FLAC_LIBRARIES}"
        )

    set(FLAC_LINK_WITH allegro_flac PARENT_SCOPE)

    install(FILES allegro5/allegro_flac.h
            DESTINATION include/allegro5
            )
endif(SUPPORT_FLAC)

#-----------------------------------------------------------------------------#

set(A5_VORBIS_SOURCES ogg.c)

if(WANT_VORBIS)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${VORBIS_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${VORBIS_LIBRARIES})
        if(NOT GP2XWIZ)
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
        else(NOT GP2XWIZ)
            check_c_source_compiles("
                #include <tremor/ivorbisfile.h>
                int main(void)
                {
                    OggVorbis_File f;
                    ov_callbacks callback;
                    vorbis_info *v = 0;
                    ov_info(&f, -1);
                    //callback = OV_CALLBACKS_NOCLOSE;
                    return 0;
                }"
                VORBIS_COMPILES)
        endif(NOT GP2XWIZ)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(VORBIS_COMPILES)
            set(SUPPORT_VORBIS 1)
            set(SUPPORT_VORBIS 1 PARENT_SCOPE)
            include_directories(${VORBIS_INCLUDE_DIR})
        endif(VORBIS_COMPILES)
    else(VORBIS_FOUND)
        message("WARNING: libvorbis not found, disabling support.")
    endif(VORBIS_FOUND)
endif(WANT_VORBIS)

if(SUPPORT_VORBIS)
    add_our_library(allegro_vorbis -${ALLEGRO_VERSION}
        "${A5_VORBIS_SOURCES}"
        "-DALLEGRO_VORBIS_SRC"
        "${AUDIO_LINK_WITH};${VORBIS_LIBRARIES}"
        )

    set(VORBIS_LINK_WITH allegro_vorbis PARENT_SCOPE)

    install(FILES allegro5/allegro_vorbis.h
            DESTINATION include/allegro5
            )
endif(SUPPORT_VORBIS)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
