include_directories(../kcm_audio)

option(WANT_FLAC "Enable FLAC support (acodec)" on)
option(WANT_VORBIS "Enable Ogg Vorbis support (acodec)" on)
option(WANT_SNDFILE "Enable WAV/AIFF/FLAC support (acodec)" on)

set(ACODEC_EXTRA_LIBS)
set(ACODEC_SOURCES acodec.c)

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
            set(SUPPORT_FLAC 1)
            set(ALLEGRO_CFG_ACODEC_FLAC 1)
            include_directories(${FLAC_INCLUDE_DIR})
            list(APPEND ACODEC_EXTRA_LIBS ${FLAC_LIBRARIES})
            list(APPEND ACODEC_SOURCES flac.c)
        endif(FLAC_COMPILES)
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
            set(SUPPORT_VORBIS 1)
            set(ALLEGRO_CFG_ACODEC_VORBIS 1)
            include_directories(${VORBIS_INCLUDE_DIR})
            list(APPEND ACODEC_EXTRA_LIBS ${VORBIS_LIBRARIES})
            list(APPEND ACODEC_SOURCES ogg.c)
        endif(VORBIS_COMPILES)
    endif(VORBIS_FOUND)
endif(WANT_VORBIS)

if(WANT_SNDFILE)
    find_package(Sndfile)
    if(SNDFILE_FOUND)
        set(SUPPORT_SNDFILE 1)
        set(ALLEGRO_CFG_ACODEC_SNDFILE 1)
        include_directories(${SNDFILE_INCLUDE_DIR})
        list(APPEND ACODEC_EXTRA_LIBS ${SNDFILE_LIBRARIES})
        list(APPEND ACODEC_SOURCES wav.c)
    endif(SNDFILE_FOUND)
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
                DESTINATION "lib${LIB_SUFFIX}"
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
        really_add_acodec_build(${nm}-static ${nm}_static STATIC "-DALLEGRO_STATICLINK")
    endif(STATIC)

endmacro(add_acodec_build)

if(SUPPORT_ACODEC)

    if(GRADE_STANDARD)
        add_acodec_build(a5_acodec
            "-O2 -funroll-loops -ffast-math -fomit-frame-pointer")
    endif(GRADE_STANDARD)

    if(GRADE_DEBUG)
        add_acodec_build(a5_acodec-debug
            "-DDEBUGMODE -g")
    endif(GRADE_DEBUG)

    if(GRADE_PROFILE)
        add_acodec_build(a5_acodec-profile
            "-pg -O2 -funroll-loops -ffast-math")
    endif(GRADE_PROFILE)

    if(GRADE_STANDARD)
        set(ACODEC_LIB_TO_LINK a5_acodec CACHE INTERNAL "internal")
    elseif(GRADE_DEBUG)
        set(ACODEC_LIB_TO_LINK a5_acodec-debug CACHE INTERNAL "internal")
    elseif(GRADE_PROFILE)
        set(ACODEC_LIB_TO_LINK a5_acodec-profile CACHE INTERNAL "internal")
    endif(GRADE_STANDARD)

    if(STATIC)
        set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_static
            CACHE INTERNAL "internal")
        set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_static PARENT_SCOPE)
        set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} ${AUDIO_LIB_TO_LINK}_static
            CACHE INTERNAL "internal")
    else(STATIC)
        set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_shared
            CACHE INTERNAL "internal")
        set(ACODEC_LINK_WITH ${ACODEC_LIB_TO_LINK}_shared PARENT_SCOPE)
        set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} ${AUDIO_LIB_TO_LINK}_shared
            CACHE INTERNAL "internal")
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

    # Install header files.

    install(FILES ${ACODEC_INCLUDE_FILES}
            DESTINATION include/allegro5
            )

endif(SUPPORT_ACODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
