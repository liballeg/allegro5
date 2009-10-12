set(IMAGE_SOURCES bmp.c iio.c pcx.c tga.c)
set(IMAGE_INCLUDE_FILES allegro5/allegro_image.h)

# Accumulate these.
set(IMAGE_LIBRARIES)
set(IMAGE_DEFINES)

if(WANT_IMAGE_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_PNG 1)
        list(APPEND IMAGE_SOURCES png.c)
        list(APPEND IMAGE_LIBRARIES ${PNG_LIBRARIES})
        list(APPEND IMAGE_DEFINES ${PNG_DEFINITIONS})
        include_directories(SYSTEM ${PNG_INCLUDE_DIR})
    else(PNG_FOUND)
        message("WARNING: libpng not found, disabling support")
    endif(PNG_FOUND)
endif(WANT_IMAGE_PNG)

if(WANT_IMAGE_JPG)
    find_package(JPEG)
    if(JPEG_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_JPG 1)
        list(APPEND IMAGE_SOURCES jpg.c)
        list(APPEND IMAGE_LIBRARIES ${JPEG_LIBRARIES})
        list(APPEND IMAGE_DEFINES ${JPEG_DEFINITIONS})
        include_directories(SYSTEM ${JPEG_INCLUDE_DIR})
    else(JPEG_FOUND)
        message("WARNING: libjpeg not found, disabling support")
    endif(JPEG_FOUND)
endif(WANT_IMAGE_JPG)

configure_file(
    allegro5/internal/aintern_image_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_image_cfg.h
    )

add_our_library(allegro_image -${ALLEGRO_VERSION}
    "${IMAGE_SOURCES}"
    "-DALLEGRO_IIO_SRC"
    "${ALLEGRO_LINK_WITH};${IMAGE_LIBRARIES}"
    )

set(IMAGE_LINK_WITH allegro_image PARENT_SCOPE)

install(FILES ${IMAGE_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
