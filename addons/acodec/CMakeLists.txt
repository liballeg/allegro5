include_directories(../audio)

option(WANT_FLAC "Enable FLAC support" on)
if(WIN32)
    option(FLAC_STATIC "Set this if linking with a static FLAC library" off)
endif()
option(WANT_VORBIS "Enable Ogg Vorbis support using libvorbis" on)
option(WANT_TREMOR "Enable Ogg Vorbis support using Tremor" off)
option(WANT_OPUS "Enable Opus support using libopus" on)
option(WANT_DUMB "Enable mod audio support using DUMB" on)
option(WANT_OPENMPT "Enable mod audio support using OpenMPT" on)
option(WANT_ACODEC_DYNAMIC_LOAD "Enable DLL loading in acodec (Windows)" off)
option(WANT_MP3 "Enable MP3 support" on)

#-----------------------------------------------------------------------------#

set(ACODEC_INCLUDE_FILES
    allegro5/allegro_acodec.h
    )

set_our_header_properties(${ACODEC_INCLUDE_FILES})

set(ACODEC_SOURCES
    acodec.c
    wav.c
    helper.c
    voc.c        # built-in enhanced port of A4 loader
    )
set(ACODEC_LIBRARIES)

# For dynamic loading, we want to make sure that CMake has found an import
# library and not a static library. We assume that the name of the DLL to load
# is the same as the import library, bar the extension.
#
# With MSVC, static libraries and import libraries share the same extension.
# Luckily the MSVC static libraries for FLAC and Vorbis are named with a
# _static suffix.
# With MinGW, static libraries end with .a, and import libraries end with
# .dll.a so we can tell them apart. (The regex for this is a bodge.)
set(WIN32_STATIC_LIB_REGEX "_static[.]|[^l][.]a")

function(get_dll_name implib dllname_var)
    if(MINGW)
        # Guess the name of dlltool from gcc.
        string(REGEX REPLACE "gcc.*" dlltool DLLTOOL ${CMAKE_C_COMPILER})
        execute_process(
            COMMAND ${DLLTOOL} -I ${implib}
            OUTPUT_VARIABLE dllname
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )
    elseif(MSVC)
        # Not sure this is the best way.
        execute_process(
            COMMAND lib /LIST ${implib}
            OUTPUT_VARIABLE output
            )
        if(output STREQUAL "")
            message("WARNING: Failed to execute lib /list")
        else()
            string(REGEX MATCH "[^\n]+[.]dll" dllname "${output}")
        endif()
    endif()
    if(NOT dllname)
        # Guess from the basename.
        get_filename_component(basename "${implib}" NAME_WE)
        set(dllname "${basename}.dll")
    endif()
    message(STATUS "DLL name for ${implib}: ${dllname}")
    set(${dllname_var} ${dllname} PARENT_SCOPE)
endfunction(get_dll_name)

set(ACODEC_CONFIGURATION_SUMMARY "")

function(acodec_summary msg yesno)
    if(${yesno})
        set(yesno "yes")
    else()
        # No's are more important, so shout them.
        set(yesno "NO")
    endif()
    set(ACODEC_CONFIGURATION_SUMMARY "${ACODEC_CONFIGURATION_SUMMARY}${msg}: ${yesno}\n" PARENT_SCOPE)
endfunction()

#
# FLAC
#

if(WANT_FLAC)
    find_package(FLAC)
    if(FLAC_FOUND)
        if(FLAC_LIBRARY MATCHES "${WIN32_STATIC_LIB_REGEX}")
            set(FLAC_STATIC 1)
        endif()
        if(FLAC_STATIC)
            set(FLAC__NO_DLL "-DFLAC__NO_DLL")
        endif()
        set(CMAKE_REQUIRED_INCLUDES ${FLAC_INCLUDE_DIR})
        if(MSVC)
            set(CMAKE_REQUIRED_LIBRARIES ${FLAC_LIBRARIES})
        else()
            # FLAC requires libm on Android, doesn't seem to hurt elsewhere either.
            set(CMAKE_REQUIRED_LIBRARIES ${FLAC_LIBRARIES} m)
        endif()
        set(CMAKE_REQUIRED_DEFINITIONS ${FLAC__NO_DLL})
        run_c_compile_test("
            #include <FLAC/stream_decoder.h>
            int main(void)
            {
                (void)FLAC__stream_decoder_new();
                return 0;
            }"
            FLAC_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        set(CMAKE_REQUIRED_DEFINITIONS)
        if(FLAC_COMPILES)
            set(SUPPORT_FLAC 1)
        endif(FLAC_COMPILES)
    endif(FLAC_FOUND)
    if(NOT SUPPORT_FLAC)
        message("WARNING: libFLAC not found or compile test failed, disabling support.")
    endif(NOT SUPPORT_FLAC)
endif(WANT_FLAC)

if(SUPPORT_FLAC)
    include_directories(SYSTEM ${FLAC_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_FLAC 1)
    list(APPEND ACODEC_SOURCES flac.c)

    list(APPEND ACODEC_INCLUDE_DIRECTORIES ${FLAC_INCLUDE_DIR})

    if(WIN32)
        if(WANT_ACODEC_DYNAMIC_LOAD)
            if(FLAC_STATIC)
                message("WARNING: Dynamic loading will be disabled for FLAC as"
                        " static library was found: ${FLAC_LIBRARY}")
            else()
                get_dll_name(${FLAC_LIBRARY} ALLEGRO_CFG_ACODEC_FLAC_DLL)
            endif()
        endif()
    endif(WIN32)

    if(NOT ALLEGRO_CFG_ACODEC_FLAC_DLL)
        list(APPEND ACODEC_LIBRARIES ${FLAC_LIBRARIES})
    endif()
endif(SUPPORT_FLAC)

acodec_summary(" - FLAC" SUPPORT_FLAC)

#
# MOD audio
#

if(WANT_DUMB)
    find_package(DUMB)
    if(DUMB_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${DUMB_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${DUMB_LIBRARIES})
        run_c_compile_test("
            #define _FILE_OFFSET_BITS 64
            #include <dumb.h>
            #if (DUMB_MAJOR_VERSION) == 1
                #error libdumb 1.0 not supported, get >= 2.0 or 0.9.3
            #endif
            int main(void)
            {
                dumb_register_stdfiles();
                return 0;
            }"
            DUMB_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(DUMB_COMPILES)
            set(SUPPORT_DUMB 1)
        endif(DUMB_COMPILES)
    endif(DUMB_FOUND)
    if(NOT SUPPORT_DUMB)
        message("WARNING: libdumb >= 2.0 or <= 0.9.3 not found or compile "
                "test failed, disabling support. See "
                "<https://github.com/kode54/dumb> for 2.0 or "
                "<http://dumb.sourceforge.net/> for 0.9.3.")
    endif(NOT SUPPORT_DUMB)
endif(WANT_DUMB)

if(SUPPORT_DUMB)
    include_directories(SYSTEM ${DUMB_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_DUMB 1)
    list(APPEND ACODEC_SOURCES dumb.c)

    list(APPEND ACODEC_INCLUDE_DIRECTORIES ${DUMB_INCLUDE_DIR})

    if(WIN32 AND WANT_ACODEC_DYNAMIC_LOAD)
        if(DUMB_LIBRARY MATCHES "${WIN32_STATIC_LIB_REGEX}")
            message("WARNING: Dynamic loading will be disabled for DUMB"
                    " as static library was found: ${DUMB_LIBRARY}")
        else()
            get_dll_name(${DUMB_LIBRARY} ALLEGRO_CFG_ACODEC_DUMB_DLL)
        endif()
    endif()

    if(NOT ALLEGRO_CFG_ACODEC_DUMB_DLL)
        list(APPEND ACODEC_LIBRARIES ${DUMB_LIBRARIES})
    endif()
endif()

acodec_summary(" - DUMB" SUPPORT_DUMB)

if(WANT_OPENMPT)
    find_package(OpenMPT)
    if(OPENMPT_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${OPENMPT_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${OPENMPT_LIBRARIES})
        run_c_compile_test("
            #include <libopenmpt/libopenmpt.h>
            #include <libopenmpt/libopenmpt_stream_callbacks_file.h>
            int main(void)
            {
                openmpt_stream_get_file_callbacks();
                return 0;
            }"
            OPENMPT_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(OPENMPT_COMPILES)
            set(SUPPORT_OPENMPT 1)
        endif()
    endif()
endif()

if(SUPPORT_OPENMPT)
    include_directories(SYSTEM ${OPENMPT_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_OPENMPT 1)
    list(APPEND ACODEC_SOURCES openmpt.c)
    list(APPEND ACODEC_INCLUDE_DIRECTORIES ${OPENMPT_INCLUDE_DIR})
    list(APPEND ACODEC_LIBRARIES ${OPENMPT_LIBRARIES})
endif()

acodec_summary(" - OpenMPT" SUPPORT_OPENMPT)

if(SUPPORT_OPENMPT OR SUPPORT_DUMB)
    list(APPEND ACODEC_SOURCES modaudio.c)
endif(SUPPORT_OPENMPT OR SUPPORT_DUMB)

#
# Vorbis/Tremor
#

if(WANT_TREMOR)
    find_package(Tremor)
    if(TREMOR_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${TREMOR_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${TREMOR_LIBRARIES})
        run_c_compile_test("
            #include <tremor/ivorbisfile.h>
            int main(void)
            {
                OggVorbis_File f;
                ov_info(&f, -1);
                return 0;
            }"
            TREMOR_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(TREMOR_COMPILES OR IPHONE OR ALLEGRO_RASPBERRYPI)
            set(SUPPORT_VORBIS 1)
        endif(TREMOR_COMPILES OR IPHONE OR ALLEGRO_RASPBERRYPI)
    endif(TREMOR_FOUND)
    if(NOT SUPPORT_VORBIS)
        message("WARNING: Tremor not found although WANT_TREMOR was specified.")
    else(NOT SUPPORT_VORBIS)
        # mimic regular libogg/libvorbis
        set(OGG_INCLUDE_DIR ${TREMOR_INCLUDE_DIR})
        set(VORBIS_INCLUDE_DIR ${TREMOR_INCLUDE_DIR})
        set(VORBIS_LIBRARIES ${TREMOR_LIBRARIES})
        set(ALLEGRO_CFG_ACODEC_TREMOR 1)
    endif(NOT SUPPORT_VORBIS)
elseif(WANT_VORBIS)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
        if(MSVC)
            set(CMAKE_REQUIRED_LIBRARIES "${VORBIS_LIBRARIES}")
        else()
            # libm is required when linking statically.
            set(CMAKE_REQUIRED_LIBRARIES "${VORBIS_LIBRARIES};m")
        endif()
        run_c_compile_test("
            #include <vorbis/vorbisfile.h>
            int main(void)
            {
                OggVorbis_File f;
                ov_callbacks callback;
                vorbis_info *v = 0;
                (void)v;
                ov_info(&f, -1);
                callback = OV_CALLBACKS_NOCLOSE;
                return 0;
            }"
            VORBIS_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(VORBIS_COMPILES)
            set(SUPPORT_VORBIS 1)
        endif()
    endif(VORBIS_FOUND)
    if(NOT SUPPORT_VORBIS)
        message("WARNING: libvorbis not found or compile test failed, disabling support.")
    endif(NOT SUPPORT_VORBIS)
endif()

if(SUPPORT_VORBIS)
    include_directories(SYSTEM ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_VORBIS 1)
    list(APPEND ACODEC_SOURCES ogg.c)

    list(APPEND ACODEC_INCLUDE_DIRECTORIES ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})

    if(WIN32 AND WANT_ACODEC_DYNAMIC_LOAD AND NOT WANT_TREMOR)
        if(VORBISFILE_LIBRARY MATCHES "${WIN32_STATIC_LIB_REGEX}")
            message("WARNING: Dynamic loading will be disabled for Vorbis"
                    " as static library was found: ${VORBISFILE_LIBRARY}")
        else()
            get_dll_name(${VORBISFILE_LIBRARY} ALLEGRO_CFG_ACODEC_VORBISFILE_DLL)
        endif()
    endif()

    if(NOT ALLEGRO_CFG_ACODEC_VORBISFILE_DLL)
        list(APPEND ACODEC_LIBRARIES ${VORBIS_LIBRARIES})
    endif()
endif(SUPPORT_VORBIS)

acodec_summary(" - Ogg/Vorbis" SUPPORT_VORBIS)

#
# Opus
#

if(WANT_OPUS)
    find_package(Opus)
    if(OPUS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${OGG_INCLUDE_DIR} ${OPUS_INCLUDE_DIR})
        if(MSVC)
            set(CMAKE_REQUIRED_LIBRARIES "${OPUS_LIBRARIES}")
        else()
            # libm is required when linking statically.
            set(CMAKE_REQUIRED_LIBRARIES "${OPUS_LIBRARIES};m")
        endif()
        run_c_compile_test("
            #include <opus/opusfile.h>
            int main(void)
            {
                OggOpusFile *f;
                OpusHead *v = 0;
                (void)v;
                op_free(f);
                return 0;
            }"
            OPUS_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(OPUS_COMPILES)
            set(SUPPORT_OPUS 1)
        endif()
    endif(OPUS_FOUND)
    if(NOT SUPPORT_OPUS)
        message("WARNING: libopus not found or compile test failed, disabling support.")
    endif(NOT SUPPORT_OPUS)
endif()

if(SUPPORT_OPUS)
    include_directories(SYSTEM ${OGG_INCLUDE_DIR} ${OPUS_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_OPUS 1)
    list(APPEND ACODEC_SOURCES opus.c)

    list(APPEND ACODEC_INCLUDE_DIRECTORIES ${OGG_INCLUDE_DIR} ${OPUS_INCLUDE_DIR})

    if(WIN32 AND WANT_ACODEC_DYNAMIC_LOAD)
        if(OPUSFILE_LIBRARY MATCHES "${WIN32_STATIC_LIB_REGEX}")
            message("WARNING: Dynamic loading will be disabled for Opus"
                    " as static library was found: ${OPUSFILE_LIBRARY}")
        else()
            get_dll_name(${OPUSFILE_LIBRARY} ALLEGRO_CFG_ACODEC_OPUSFILE_DLL)
        endif()
    endif()

    if(NOT ALLEGRO_CFG_ACODEC_OPUSFILE_DLL)
        list(APPEND ACODEC_LIBRARIES ${OPUS_LIBRARIES})
    endif()
endif(SUPPORT_OPUS)

acodec_summary(" - Opus" SUPPORT_OPUS)

#
# MP3
#
if(WANT_MP3)
    find_package(MiniMP3)
    if(MINIMP3_FOUND)
        include_directories(SYSTEM ${MINIMP3_INCLUDE_DIRS})
        set(ALLEGRO_CFG_ACODEC_MP3 1)
        list(APPEND ACODEC_SOURCES mp3.c)
    endif(MINIMP3_FOUND)
    if(NOT MINIMP3_FOUND)
        message("WARNING: minimp3 was not found")
    endif(NOT MINIMP3_FOUND)
endif()

acodec_summary(" - MP3" MINIMP3_FOUND)


configure_file(
    allegro5/internal/aintern_acodec_cfg.h.cmake
    ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_acodec_cfg.h
    )

add_our_addon_library(allegro_acodec
    AllegroAcodec-${ALLEGRO_SOVERSION}
    "${ACODEC_SOURCES};${ACODEC_INCLUDE_FILES}"
    "-DALLEGRO_ACODEC_SRC ${FLAC__NO_DLL}"
    "${AUDIO_LINK_WITH};${ACODEC_LIBRARIES}"
    )

install_our_headers(${ACODEC_INCLUDE_FILES})

add_addon(acodec)

set(ACODEC_CONFIGURATION_SUMMARY "${ACODEC_CONFIGURATION_SUMMARY}" PARENT_SCOPE)


#-----------------------------------------------------------------------------#
# vim: set ts=8 sts=4 sw=4 et:
