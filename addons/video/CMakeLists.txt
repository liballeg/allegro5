option(WANT_OGG_VIDEO "Enable Ogg video (requires Theora and Vorbis)" on)

set(VIDEO_SOURCES
    video.c
    )

set(VIDEO_INCLUDE_FILES allegro5/allegro_video.h)

include_directories(../audio ../image .)

set_our_header_properties(${VIDEO_INCLUDE_FILES})

# For monolith build.
set(VIDEO_INCLUDE_DIRECTORIES)

if(WANT_OGG_VIDEO)
    find_package(Theora)
    find_package(Vorbis)
    if(THEORA_FOUND AND VORBIS_FOUND)
        set(SUPPORT_OGG_VIDEO 1)
    endif()
endif(WANT_OGG_VIDEO)

if(SUPPORT_OGG_VIDEO)
    list(APPEND VIDEO_INCLUDE_DIRECTORIES
        ${OGG_INCLUDE_DIR}
        ${VORBIS_INCLUDE_DIR}
        ${THEORA_INCLUDE_DIR}
        )
    list(APPEND VIDEO_SOURCES ogv.c)
    set(SUPPORT_VIDEO 1)
    set(A5O_CFG_VIDEO_HAVE_OGV 1)
    set(VIDEO_LIBRARIES ${VIDEO_LIBRARIES} ${THEORA_LIBRARIES} ${VORBIS_LIBRARIES})
endif(SUPPORT_OGG_VIDEO)

if(NOT SUPPORT_VIDEO)
    message("WARNING: allegro_video wanted but no supported backend found")
    return()
endif(NOT SUPPORT_VIDEO)

include_directories(SYSTEM ${VIDEO_INCLUDE_DIRECTORIES})

configure_file(
    allegro5/internal/aintern_video_cfg.h.cmake
    ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_video_cfg.h
    )

add_our_addon_library(allegro_video
    AllegroVideo-${A5O_SOVERSION}
    "${VIDEO_SOURCES};${VIDEO_INCLUDE_FILES}"
    "-DA5O_VIDEO_SRC"
    "${A5O_LINK_WITH};${AUDIO_LINK_WITH};${VIDEO_LIBRARIES}"
    )

install_our_headers(${VIDEO_INCLUDE_FILES})

add_addon(video)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
