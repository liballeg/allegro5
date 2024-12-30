option(WANT_NATIVE_IMAGE_LOADER "Enable the native platform image loader (if available)" on)

set(IMAGE_SOURCES bmp.c iio.c pcx.c tga.c dds.c identify.c)
set(IMAGE_INCLUDE_FILES allegro5/allegro_image.h)

set_our_header_properties(${IMAGE_INCLUDE_FILES})

# Accumulate these.
set(IMAGE_LIBRARIES)
set(IMAGE_DEFINES)

set(IMAGE_CONFIGURATION_SUMMARY "")

function(image_summary msg yesno)
    if(${yesno})
        set(yesno "yes")
    else()
        # No's are more important, so shout them.
        set(yesno "NO")
    endif()
    set(IMAGE_CONFIGURATION_SUMMARY "${IMAGE_CONFIGURATION_SUMMARY}${msg}: ${yesno}\n" PARENT_SCOPE)
endfunction()

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
            run_cxx_compile_test("
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
            list(APPEND IMAGE_INCLUDE_DIRECTORIES ${GDIPLUS_INCLUDE_DIR})
            include_directories(SYSTEM ${GDIPLUS_INCLUDE_DIR})
        else(SUPPORT_GDIPLUS)
            message("WARNING: cannot use GDI+. Will try other libraries.")
        endif(SUPPORT_GDIPLUS)
    endif(WIN32)

    if (ALLEGRO_MACOSX)
        set(ALLEGRO_CFG_IIO_SUPPORT_PNG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_JPG 1)
        list(APPEND IMAGE_SOURCES macosx.m)
    endif(ALLEGRO_MACOSX)

    if(ANDROID)
        set(ALLEGRO_CFG_IIO_HAVE_ANDROID 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_JPG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_PNG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_WEBP 1)
        list(APPEND IMAGE_SOURCES android.c)
    endif(ANDROID)

    if(IPHONE)
        list(APPEND IMAGE_SOURCES iphone.m)
    endif(IPHONE)
endif(WANT_NATIVE_IMAGE_LOADER)

if(WIN32)
    image_summary(" - GDI+" SUPPORT_GDIPLUS)
elseif(MACOSX OR ANDROID OR IPHONE)
    image_summary(" - Native image loader" WANT_NATIVE_IMAGE_LOADER)
endif()

# Now look for third party libraries to handle the unsupported formats
if(WANT_IMAGE_FREEIMAGE)
    find_package(FreeImage)
    if(FREEIMAGE_FOUND)
        # HAVE_FREEIMAGE means libfreeimage is available (and should be used)
        set(ALLEGRO_CFG_IIO_HAVE_FREEIMAGE 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_FREEIMAGE 1)
        list(APPEND IMAGE_SOURCES freeimage.c)
        list(APPEND IMAGE_LIBRARIES ${FREEIMAGE_LIBRARIES})
        list(APPEND IMAGE_DEFINES ${FREEIMAGE_DEFINITIONS})
        list(APPEND IMAGE_INCLUDE_DIRECTORIES ${FREEIMAGE_INCLUDE_PATH})
        include_directories(SYSTEM ${FREEIMAGE_INCLUDE_PATH})
    else(FREEIMAGE_FOUND)
        message("WARNING: FreeImage not found, disabling support")
    endif(FREEIMAGE_FOUND)
endif(WANT_IMAGE_FREEIMAGE)

image_summary(" - FreeImage" ALLEGRO_CFG_IIO_SUPPORT_FREEIMAGE)

if(WANT_IMAGE_PNG AND NOT ALLEGRO_CFG_IIO_SUPPORT_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        # HAVE_PNG means libpng is available (and should be used)
        set(ALLEGRO_CFG_IIO_HAVE_PNG 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_PNG 1)
        list(APPEND IMAGE_SOURCES png.c)
        list(APPEND IMAGE_LIBRARIES ${PNG_LIBRARIES})
        list(APPEND IMAGE_DEFINES ${PNG_DEFINITIONS})
        list(APPEND IMAGE_INCLUDE_DIRECTORIES ${PNG_INCLUDE_DIR})
        include_directories(SYSTEM ${PNG_INCLUDE_DIR})
    else(PNG_FOUND)
        message("WARNING: libpng not found, disabling support")
    endif(PNG_FOUND)
endif(WANT_IMAGE_PNG AND NOT ALLEGRO_CFG_IIO_SUPPORT_PNG)

image_summary(" - libpng" PNG_FOUND)

if(WANT_IMAGE_JPG AND NOT ALLEGRO_CFG_IIO_SUPPORT_JPG)
    find_package(JPEG)
    if(JPEG_FOUND AND MINGW)
        set(CMAKE_REQUIRED_INCLUDES ${JPEG_INCLUDE_DIR})
        run_c_compile_test("
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

image_summary(" - libjpeg" SUPPORT_JPEG)

if(WANT_IMAGE_WEBP AND NOT ALLEGRO_CFG_IIO_SUPPORT_WEBP)
    find_package(WebP)
    if(WEBP_FOUND)
        set(ALLEGRO_CFG_IIO_HAVE_WEBP 1)
        set(ALLEGRO_CFG_IIO_SUPPORT_WEBP 1)
        list(APPEND IMAGE_SOURCES webp.c)
        list(APPEND IMAGE_LIBRARIES ${WEBP_LIBRARIES})
        list(APPEND IMAGE_INCLUDE_DIRECTORIES ${WEBP_INCLUDE_DIRS})
        include_directories(SYSTEM ${WEBP_INCLUDE_DIRS})
    else(WEBP_FOUND)
        message("WARNING: libwebp not found, disabling support")
    endif(WEBP_FOUND)
endif(WANT_IMAGE_WEBP AND NOT ALLEGRO_CFG_IIO_SUPPORT_WEBP)

image_summary(" - libwebp" WEBP_FOUND)

configure_file(
    allegro5/internal/aintern_image_cfg.h.cmake
    ${PROJECT_BINARY_DIR}/include/allegro5/internal/aintern_image_cfg.h
    )

add_our_addon_library(allegro_image
    AllegroImage-${ALLEGRO_SOVERSION}
    "${IMAGE_SOURCES};${IMAGE_INCLUDE_FILES}"
    "-DALLEGRO_IIO_SRC"
    "${ALLEGRO_LINK_WITH};${IMAGE_LIBRARIES}"
    )

install_our_headers(${IMAGE_INCLUDE_FILES})

add_addon(image)

set(IMAGE_CONFIGURATION_SUMMARY "${IMAGE_CONFIGURATION_SUMMARY}" PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
