option(WANT_NATIVE_IMAGE_LOADER "Enable the native platform image loader (if available)" on)

set(IMAGE_SOURCES bmp.c iio.c pcx.c tga.c)
set(IMAGE_INCLUDE_FILES allegro5/allegro_image.h)

set_our_header_properties(${IMAGE_INCLUDE_FILES})

# Accumulate these.
set(IMAGE_LIBRARIES)
set(IMAGE_DEFINES)

# ALLEGRO_CFG_IIO_HAVE_* are the available libraries.
# ALLEGRO_CFG_IIO_SUPPORT_* are the supported formats.

# First look for native libraries and mark any supported image
# type as found, so that the associated third party libraries 
# don't need to be used.

if(WANT_NATIVE_IMAGE_LOADER)
    set(ALLEGRO_CFG_WANT_NATIVE_IMAGE_LOADER 1)

    if(WIN32)
        find_package(GDIPLUS)
        if(GDIPLUS_FOUND)
            set(CMAKE_REQUIRED_DEFINITIONS -DGDIPLUS_LOWERCASE=${GDIPLUS_LOWERCASE})
            set(CMAKE_REQUIRED_INCLUDES ${GDIPLUS_INCLUDE_DIR})
            set(CMAKE_REQUIRED_LIBRARIES ${GDIPLUS_LIBRARY})
            check_cxx_source_compiles("
                #include <windows.h>
                #include <objidl.h>
                #if GDIPLUS_LOWERCASE
                    #include <gdiplus.h>
                #else
                    #include <GdiPlus.h>
                #endif
                using namespace Gdiplus;
                int main(void)
                {
                    int pf = PixelFormat32bppARGB;
                    return 0;
                }
                "
                SUPPORT_GDIPLUS)
        endif(GDIPLUS_FOUND)
        if(SUPPORT_GDIPLUS)
            set(ALLEGRO_CFG_IIO_HAVE_GDIPLUS 1)
            set(ALLEGRO_CFG_IIO_HAVE_GDIPLUS_LOWERCASE_H ${GDIPLUS_LOWERCASE})
            set(ALLEGRO_CFG_IIO_SUPPORT_PNG 1)
            set(ALLEGRO_CFG_IIO_SUPPORT_JPG 1)
            list(APPEND IMAGE_SOURCES gdiplus.cpp)
            list(APPEND IMAGE_LIBRARIES ${GDIPLUS_LIBRARIES})
            if(MINGW)
                list(APPEND IMAGE_LIBRARIES uuid)
            endif(MINGW)
            list(APPEND IMAGE_DEFINES ${GDIPLUS_DEFINITIONS})
            include_directories(SYSTEM ${GDIPLUS_INCLUDE_DIR})            
        else(SUPPORT_GDIPLUS)
            message("WARNING: cannot use GDI+. Will try other libraries.")
        endif(SUPPORT_GDIPLUS)
    endif(WIN32)

    if (MACOSX)
        set(ALLEGRO_CFG_IIO_SUPPORT_PNG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_JPG 1)
        list(APPEND IMAGE_SOURCES macosx.m)
    endif(MACOSX)

    if(IPHONE)
        list(APPEND IMAGE_SOURCES iphone.m)
    endif(IPHONE)
endif(WANT_NATIVE_IMAGE_LOADER)

# Now look for third party libraries to handle the unsupported formats

if(WANT_IMAGE_PNG AND NOT ALLEGRO_CFG_IIO_SUPPORT_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        # HAVE_PNG means libpng is available (and should be used)
        set(ALLEGRO_CFG_IIO_HAVE_PNG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_PNG 1)
        list(APPEND IMAGE_SOURCES png.c)
        list(APPEND IMAGE_LIBRARIES ${PNG_LIBRARIES})
        list(APPEND IMAGE_DEFINES ${PNG_DEFINITIONS})
        include_directories(SYSTEM ${PNG_INCLUDE_DIR})
    else(PNG_FOUND)
        message("WARNING: libpng not found, disabling support")
    endif(PNG_FOUND)
endif(WANT_IMAGE_PNG AND NOT ALLEGRO_CFG_IIO_SUPPORT_PNG)

if(WANT_IMAGE_JPG AND NOT ALLEGRO_CFG_IIO_SUPPORT_JPG)
    find_package(JPEG)
    if(JPEG_FOUND AND MINGW)
        set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
        check_c_source_compiles("
            #include <stdio.h>
            #include <windows.h>
            #include <jpeglib.h>
            #include <jerror.h>
            int main(void) {
                return 0;
            }"
            JPEG_COMPILES)
        set(CMAKE_REQUIRED_INCLUDES)
        set(SUPPORT_JPEG ${JPEG_COMPILES})
    else()
        set(SUPPORT_JPEG ${JPEG_FOUND})
    endif()
    if(SUPPORT_JPEG)
        # HAVE_JPG means libjpeg is available (and should be used)
        set(ALLEGRO_CFG_IIO_HAVE_JPG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_JPG 1)
        list(APPEND IMAGE_SOURCES jpg.c)
        list(APPEND IMAGE_LIBRARIES ${JPEG_LIBRARIES})
        list(APPEND IMAGE_DEFINES ${JPEG_DEFINITIONS})
        list(APPEND IMAGE_INCLUDE_DIRECTORIES ${JPEG_INCLUDE_DIR})
        include_directories(SYSTEM ${JPEG_INCLUDE_DIR})
    else()
        if(MINGW AND JPEG_FOUND AND NOT JPEG_COMPILES)
            message("WARNING: libjpeg found but the headers appear to "
                "conflict with your MinGW headers, so disabling support. "
                "Try a later version.")
        else()
            message("WARNING: libjpeg not found, disabling support")
        endif()
    endif()
endif(WANT_IMAGE_JPG AND NOT ALLEGRO_CFG_IIO_SUPPORT_JPG)

configure_file(
    allegro5/internal/aintern_image_cfg.h.cmake
    ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_image_cfg.h
    )

add_our_library(allegro_image
    "${IMAGE_SOURCES};${IMAGE_INCLUDE_FILES}"
    "-DALLEGRO_IIO_SRC"
    "${ALLEGRO_LINK_WITH};${IMAGE_LIBRARIES}"
    )

set_our_framework_properties(allegro_image AllegroImage-${ALLEGRO_SOVERSION})

install_our_library(allegro_image)
install_our_headers(${IMAGE_INCLUDE_FILES})

set(IMAGE_LINK_WITH allegro_image PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
