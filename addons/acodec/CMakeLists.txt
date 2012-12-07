include_directories(../audio)

option(WANT_FLAC "Enable FLAC support" on)
option(WANT_VORBIS "Enable Ogg Vorbis support using libvorbis" on)
option(WANT_TREMOR "Enable Ogg Vorbis support using Tremor" off)
option(WANT_MODAUDIO "Enable MOD Audio support" on)
option(WANT_ACODEC_DYNAMIC_LOAD "Enable DLL loading in acodec (Windows)" on)

#-----------------------------------------------------------------------------#

set(ACODEC_INCLUDE_FILES
    allegro5/allegro_acodec.h
    )

set_our_header_properties(${ACODEC_INCLUDE_FILES})

set(ACODEC_SOURCES
    acodec.c
    wav.c
    helper.c
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

#
# FLAC
#

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

    if(WIN32)
        option(FLAC_STATIC "Set this if linking with a static FLAC library" off)
        if(FLAC_STATIC)
            set(FLAC__NO_DLL "-DFLAC__NO_DLL")
        endif()

        if(NOT FLAC_STATIC AND FLAC_LIBRARY MATCHES "${WIN32_STATIC_LIB_REGEX}")
            set(FLAC_STATIC 1)
            set(FLAC__NO_DLL "-DFLAC__NO_DLL")
        endif()
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

#
# MOD audio
#

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
        endif(DUMB_COMPILES)
    endif(DUMB_FOUND)
    if(NOT SUPPORT_MODAUDIO)
        message("WARNING: libdumb not found or compile test failed, "
                "disabling support. <http://dumb.sourceforge.net/>")
    endif(NOT SUPPORT_MODAUDIO)
endif(WANT_MODAUDIO)

if(SUPPORT_MODAUDIO)
    include_directories(SYSTEM ${DUMB_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_MODAUDIO 1)
    list(APPEND ACODEC_SOURCES modaudio.c)

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

#
# Vorbis/Tremor
#

if(WANT_TREMOR)
    find_package(Tremor)
    if(TREMOR_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${TREMOR_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${TREMOR_LIBRARIES})
        check_c_source_compiles("
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
        if(TREMOR_COMPILES OR IPHONE)
            set(SUPPORT_VORBIS 1)
        endif(TREMOR_COMPILES OR IPHONE)
    endif(TREMOR_FOUND)
    if(NOT SUPPORT_VORBIS)
        message("WARNING: Tremor not found although WANT_TREMOR was specified.")
    else(NOT SUPPORT_VORBIS)
        # mimic regular libogg/libvorbis
        set(OGG_INCLUDE_DIR ${TREMOR_INCLUDE_DIR})
        set(VORBIS_INCLUDE_DIR )
        set(VORBIS_LIBRARIES ${TREMOR_LIBRARIES})
        set(ALLEGRO_CFG_ACODEC_TREMOR 1)
    endif(NOT SUPPORT_VORBIS)
elseif(WANT_VORBIS)
    find_package(Vorbis)
    if(VORBIS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
        if(COMPILER_GCC)
            # libm is required when linking statically.
            set(CMAKE_REQUIRED_LIBRARIES "${VORBIS_LIBRARIES};m")
        else()
            set(CMAKE_REQUIRED_LIBRARIES "${VORBIS_LIBRARIES}")
        endif(COMPILER_GCC)
        check_c_source_compiles("
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
        endif(VORBIS_COMPILES)
    endif(VORBIS_FOUND)
    if(NOT SUPPORT_VORBIS)
        message("WARNING: libvorbis not found or compile test failed, disabling support.")
    endif(NOT SUPPORT_VORBIS)
endif()

if(SUPPORT_VORBIS)
    include_directories(SYSTEM ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
    set(ALLEGRO_CFG_ACODEC_VORBIS 1)
    list(APPEND ACODEC_SOURCES ogg.c)

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
