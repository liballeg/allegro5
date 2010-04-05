include_directories(../audio)

option(WANT_FLAC "Enable FLAC support" on)
option(WANT_VORBIS "Enable Ogg Vorbis support" on)
option(WANT_MODAUDIO "Enable MOD Audio support" on)

#-----------------------------------------------------------------------------#

set(A5_FLAC_SOURCES flac.c)

set(A5_FLAC_INCLUDE_FILES
    allegro5/allegro_flac.h
    )

set_our_header_properties(${A5_FLAC_INCLUDE_FILES})

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
            set(SUPPORT_FLAC 1)
            set(SUPPORT_FLAC 1 PARENT_SCOPE)
            include_directories(SYSTEM ${FLAC_INCLUDE_DIR})
        endif(FLAC_COMPILES)
    else(FLAC_FOUND)
        message("WARNING: libFLAC not found, disabling support.")
    endif(FLAC_FOUND)
endif(WANT_FLAC)

if(SUPPORT_FLAC)
    add_our_library(allegro_flac
        "${A5_FLAC_SOURCES};${A5_FLAC_INCLUDE_FILES}"
        "-DALLEGRO_FLAC_SRC ${FLAC__NO_DLL}"
        "${AUDIO_LINK_WITH};${FLAC_LIBRARIES}"
        )

    set_our_framework_properties(allegro_flac AllegroFlac)

    install_our_library(allegro_flac)
    install_our_headers(${A5_FLAC_INCLUDE_FILES})

    set(FLAC_LINK_WITH allegro_flac PARENT_SCOPE)
endif(SUPPORT_FLAC)

#-----------------------------------------------------------------------------#

set(A5_VORBIS_SOURCES ogg.c)

set(A5_VORBIS_INCLUDE_FILES
    allegro5/allegro_vorbis.h
    )

set_our_header_properties(${A5_VORBIS_INCLUDE_FILES})

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
            set(SUPPORT_VORBIS 1)
            set(SUPPORT_VORBIS 1 PARENT_SCOPE)
            include_directories(SYSTEM ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
        endif(VORBIS_COMPILES)
    else(VORBIS_FOUND)
        message("WARNING: libvorbis not found, disabling support.")
    endif(VORBIS_FOUND)
endif(WANT_VORBIS)

if(SUPPORT_VORBIS)
    add_our_library(allegro_vorbis
        "${A5_VORBIS_SOURCES};${A5_VORBIS_INCLUDE_FILES}"
        "-DALLEGRO_VORBIS_SRC"
        "${AUDIO_LINK_WITH};${VORBIS_LIBRARIES}"
        )

    set_our_framework_properties(allegro_vorbis AllegroVorbis-${ALLEGRO_SOVERSION})

    install_our_library(allegro_vorbis)
    install_our_headers(${A5_VORBIS_INCLUDE_FILES})

    set(VORBIS_LINK_WITH allegro_vorbis PARENT_SCOPE)
endif(SUPPORT_VORBIS)

#-----------------------------------------------------------------------------#

set(A5_MODAUDIO_SOURCES modaudio.c)

set(A5_MODAUDIO_INCLUDE_FILES
    allegro5/allegro_modaudio.h
    )

set_our_header_properties(${A5_MODAUDIO_INCLUDE_FILES})

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
            set(SUPPORT_MODAUDIO 1)
            set(SUPPORT_MODAUDIO 1 PARENT_SCOPE)
            include_directories(SYSTEM ${FLAC_INCLUDE_DIR})
        endif(DUMB_COMPILES)
    else(DUMB_FOUND)
        message("WARNING: libdumb not found, disabling support. <http://dumb.sourceforge.net/>")
    endif(DUMB_FOUND)
endif(WANT_MODAUDIO)

if(SUPPORT_MODAUDIO)
    add_our_library(allegro_modaudio
        "${A5_MODAUDIO_SOURCES};${A5_MODAUDIO_INCLUDE_FILES}"
        "-DALLEGRO_MODAUDIO_SRC"
        "${AUDIO_LINK_WITH};${DUMB_LIBRARIES}"
        )

    set_our_framework_properties(allegro_modaudio AllegroMODAudio)

    install_our_library(allegro_modaudio)
    install_our_headers(${A5_MODAUDIO_INCLUDE_FILES})

    set(MODAUDIO_LINK_WITH allegro_modaudio PARENT_SCOPE)
endif(SUPPORT_MODAUDIO)

#-----------------------------------------------------------------------------#

#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
