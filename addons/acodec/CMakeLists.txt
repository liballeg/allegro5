include_directories(../audio)

option(WANT_FLAC "Enable FLAC support" on)
option(WANT_VORBIS "Enable Ogg Vorbis support" on)
option(WANT_MODAUDIO "Enable MOD Audio support" on)

#-----------------------------------------------------------------------------#

set(ACODEC_INCLUDE_FILES
    allegro5/allegro_acodec.h
    )

set_our_header_properties(${ACODEC_INCLUDE_FILES})

set(ACODEC_SOURCES
    acodec.c
    wav.c
    )
set(ACODEC_LIBRARIES)

if(COMPILER_MSVC)
    option(FLAC_STATIC "Set this if linking with a static FLAC library" off)
    if(FLAC_STATIC)
        set(FLAC__NO_DLL "-DFLAC__NO_DLL")
    endif()
endif(COMPILER_MSVC)

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
            list(APPEND ACODEC_SOURCES flac.c)
            list(APPEND ACODEC_LIBRARIES ${FLAC_LIBRARIES})
            include_directories(SYSTEM ${FLAC_INCLUDE_DIR})
        endif(FLAC_COMPILES)
    else(FLAC_FOUND)
        message("WARNING: libFLAC not found, disabling support.")
    endif(FLAC_FOUND)
endif(WANT_FLAC)

if(WANT_MODAUDIO)
    find_package(DUMB)
    if(DUMB_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${DUMB_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${DUMB_LIBRARIES})
        check_c_source_compiles("
            #include <dumb.h>
            int main(void)
            {
                dumb_register_stdfiles();
                return 0;
            }"
            DUMB_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(DUMB_COMPILES)
            set(ALLEGRO_CFG_ACODEC_MODAUDIO 1)
            list(APPEND ACODEC_SOURCES modaudio.c)
            list(APPEND ACODEC_LIBRARIES ${DUMB_LIBRARIES})
            include_directories(SYSTEM ${DUMB_INCLUDE_DIR})
        endif(DUMB_COMPILES)
    else(DUMB_FOUND)
        message("WARNING: libdumb not found, disabling support. <http://dumb.sourceforge.net/>")
    endif(DUMB_FOUND)
endif(WANT_MODAUDIO)

if(WANT_VORBIS)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
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
            set(ALLEGRO_CFG_ACODEC_VORBIS 1)
            list(APPEND ACODEC_SOURCES ogg.c)
            list(APPEND ACODEC_LIBRARIES ${VORBIS_LIBRARIES})
            include_directories(SYSTEM ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
        endif(VORBIS_COMPILES)
    else(VORBIS_FOUND)
        message("WARNING: libvorbis not found, disabling support.")
    endif(VORBIS_FOUND)
endif(WANT_VORBIS)

configure_file(
    allegro5/internal/aintern_acodec_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_acodec_cfg.h
    )

add_our_library(allegro_acodec
    "${ACODEC_SOURCES};${ACODEC_INCLUDE_FILES}"
    "-DALLEGRO_ACODEC_SRC ${FLAC__NO_DLL}"
    "${AUDIO_LINK_WITH};${ACODEC_LIBRARIES}"
    )

set_our_framework_properties(allegro_acodec AllegroAcodec-${ALLEGRO_SOVERSION})

install_our_library(allegro_acodec)
install_our_headers(${ACODEC_INCLUDE_FILES})

set(ACODEC_LINK_WITH allegro_acodec PARENT_SCOPE)
set(SUPPORT_ACODEC 1 PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
