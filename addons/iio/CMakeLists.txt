set(IIO_SOURCES bmp.c iio.c pcx.c tga.c)
set(IIO_INCLUDE_FILES allegro5/allegro_iio.h)

# Accumulate these.
set(IIO_LIBRARIES)
set(IIO_DEFINES)

if(WANT_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_PNG 1)
        list(APPEND IIO_SOURCES png.c)
        list(APPEND IIO_LIBRARIES ${PNG_LIBRARIES})
        list(APPEND IIO_DEFINES ${PNG_DEFINITIONS})
        include_directories(${PNG_INCLUDE_DIR})
    else(PNG_FOUND)
        message("WARNING: libpng not found, disabling support")
    endif(PNG_FOUND)
endif(WANT_PNG)

if(WANT_JPG)
    find_package(JPEG)
    if(JPEG_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_JPG 1)
        list(APPEND IIO_SOURCES jpg.c)
        list(APPEND IIO_LIBRARIES ${JPEG_LIBRARIES})
        list(APPEND IIO_DEFINES ${JPEG_DEFINITIONS})
        include_directories(${JPEG_INCLUDE_DIR})
    else(JPEG_FOUND)
        message("WARNING: libjpeg not found, disabling support")
    endif(JPEG_FOUND)
endif(WANT_JPG)

configure_file(
    allegro5/internal/aintern_iio_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_iio_cfg.h
    )

add_our_library(allegro_iio -${ALLEGRO_VERSION}
    "${IIO_SOURCES}"
    "-DA5_IIO_SRC"
    "${ALLEGRO_LINK_WITH};${IIO_LIBRARIES}"
    )

set(IIO_LINK_WITH allegro_iio PARENT_SCOPE)

install(FILES ${IIO_INCLUDE_FILES}
        DESTINATION include/allegro5
        )

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
