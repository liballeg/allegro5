set(ICODEC_INCLUDE_FILES allegro5/icodec.h)

find_package(ImageMagick COMPONENTS Magick++)
if(ImageMagick_FOUND)
    set(SUPPORT_MAGICK 1)
    include_directories(${ImageMagick_INCLUDE_DIRS})
    list(APPEND ICODEC_EXTRA_LIBS ${ImageMagick_LIBRARIES})
    list(APPEND ICODEC_SOURCES icodec.cpp)
endif(ImageMagick_FOUND)

if(SUPPORT_MAGICK)
    set(SUPPORT_ICODEC 1)
    set(SUPPORT_ICODEC 1 PARENT_SCOPE)
endif(SUPPORT_MAGICK)

if(SUPPORT_ICODEC)
    add_addon_library(a5_icodec -${ALLEGRO_VERSION}
        "${ICODEC_SOURCES}"
        "-DA5_ICODEC_SRC"
        "${ICODEC_EXTRA_LIBS};${ALLEGRO_LINK_WITH}"
        )

    set(ICODEC_LINK_WITH a5_icodec PARENT_SCOPE)

    install(FILES ${ICODEC_INCLUDE_FILES}
            DESTINATION include/allegro5
            )
endif(SUPPORT_ICODEC)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
